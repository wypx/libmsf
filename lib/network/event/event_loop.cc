/**************************************************************************
 *
 * Copyright (c) 2017-2021, luotang.me <wypx520@gmail.com>, China.
 * All rights reserved.
 *
 * Distributed under the terms of the GNU General Public License v2.
 *
 * This software is provided 'as is' with no explicit or implied warranties
 * in respect of its properties, including, but not limited to, correctness
 * and/or fitness for purpose.
 *
 **************************************************************************/
#include "event_loop.h"

#include <assert.h>
#include <base/logging.h>
#include <sys/eventfd.h>

#include <memory>

#include "epoll_poller.h"
#include "iouring_poller.h"
#include "event.h"
#include "sock_utils.h"
#include "thread.h"

using namespace MSF;

namespace MSF {

__thread EventLoop* t_loopInThisThread = nullptr;

const int kPollTimeMs = 10000;

EventLoop::EventLoop()
    : thread_id_(CurrentThread::tid()), poller_(new EPollPoller(this)) {
  LOG(TRACE) << "eventloop created " << this << " in thread " << thread_id_;
  if (t_loopInThisThread) {
    LOG(FATAL) << "another eventloop " << t_loopInThisThread
               << " exists in this thread " << thread_id_;
  } else {
    t_loopInThisThread = this;
  }

  // (0, EFD_NONBLOCK | EFD_CLOEXEC | EFD_SEMAPHORE);
  wakeup_fd_ = CreateEventFd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  assert(wakeup_fd_ > 0);
  wakeup_event_ = std::make_unique<Event>(this, wakeup_fd_);
  wakeup_event_->SetReadCallback(std::bind(&EventLoop::HandleWakeup, this));
  // we are always reading the wakeupfd
  UpdateEvent(wakeup_event_.get());

  timer_.reset(new HeapTimer(this, 1024));
  mem_pool_.reset(new MemPool());
}

EventLoop::~EventLoop() {
  LOG(INFO) << "eventloop " << this << " of thread " << thread_id_
            << " destructs in thread " << CurrentThread::tid();
  RemoveEvent(wakeup_event_.get());
  CloseSocket(wakeup_fd_);
  t_loopInThisThread = nullptr;

  //清理所有的事件?
}

void EventLoop::EnterLoop() {
  assert(!looping_);
  AssertInLoopThread();

  uint32_t next_exp_time = 1;
  looping_ = true;
  quit_ = false;  // FIXME: what if someone calls quit() before loop() ?
  LOG(INFO) << "eventloop " << this << " start looping";

  while (!quit_) {
    active_events_.clear();
    int time = poller_->Poll(next_exp_time, &active_events_);
    ++iteration_;
#if 0
    PrintActiveEvents();
#endif
    next_exp_time = timer_->TimerInLoop();
    // LOG_WARN << "NextExpTime: " << next_exp_time;
    if (next_exp_time == 0) {
      next_exp_time = kPollTimeMs;
    }
    // TODO sort channel by priority
    event_handling_ = true;
    for (Event* ev : active_events_) {
      curr_active_event_ = ev;
      curr_active_event_->HandleEvent(time);
    }
    curr_active_event_ = nullptr;
    event_handling_ = false;
    DoPendingFunctors();
  }

  LOG(INFO) << "eventloop " << this << " stop looping";
  looping_ = false;
}

void EventLoop::QuitLoop() {
  quit_ = true;
  // There is a chance that loop() just executes while(!quit_) and exits,
  // then EventLoop destructs, then we are accessing an invalid object.
  // Can be fixed using mutex_ in both places.
  if (!IsInLoopThread()) {
    Wakeup();
  }
}

void EventLoop::RunInLoop(Functor cb) {
  if (IsInLoopThread()) {
    cb();
  } else {
    QueueInLoop(std::move(cb));
  }
}

void EventLoop::QueueInLoop(Functor cb) {
  {
    std::lock_guard<std::mutex> lock(_mutex);
    pending_functors_.push_back(std::move(cb));
  }

  if (!IsInLoopThread() || calling_pending_functors_) {
    Wakeup();
  }
}

size_t EventLoop::QueueSize() const {
  std::lock_guard<std::mutex> lock(_mutex);
  return pending_functors_.size();
}

TimerId EventLoop::RunAt(const double time, const TimerCb& cb) {
  timer_->AddTimer(cb, time, TIMER_ONESHOT);
  Wakeup();
  return 0;
}

TimerId EventLoop::RunAfter(const double delay, const TimerCb& cb) {
  timer_->AddTimer(cb, delay, TIMER_ONESHOT);
  Wakeup();
  return 0;
}

TimerId EventLoop::RunEvery(const double interval, const TimerCb& cb) {
  timer_->AddTimer(cb, interval, TIMER_PERSIST);
  Wakeup();
  return 0;
}

void EventLoop::Cancel(uint64_t timerId) {
  // return timerQueue_->cancel(timerId);
  // timer_->addTimer(cb, interval, TIMER_PERSIST);
  return;
}

void EventLoop::UpdateEvent(Event* ev) {
  assert(ev->OwnerLoop() == this);
  AssertInLoopThread();
  poller_->UpdateEvent(ev);
}

void EventLoop::RemoveEvent(Event* ev) {
  assert(ev->OwnerLoop() == this);
  AssertInLoopThread();
  if (event_handling_) {
    assert(curr_active_event_ == ev ||
           std::find(active_events_.begin(), active_events_.end(), ev) ==
               active_events_.end());
  }
  poller_->RemoveEvent(ev);
}

bool EventLoop::HasEvent(Event* ev) {
  assert(ev->OwnerLoop() == this);
  AssertInLoopThread();
  return poller_->HasEvent(ev);
}

bool EventLoop::IsInLoopThread() const {
  return thread_id_ == MSF::CurrentThread::tid();
}

void EventLoop::AbortNotInLoopThread() {
  LOG(ERROR) << "not in loop thread - eventloop " << this
             << " was created in threadId_ = " << thread_id_
             << ", current thread id = " << CurrentThread::tid();
}

void EventLoop::Wakeup() {
  uint64_t one = 1;
  ssize_t n = ::write(wakeup_fd_, &one, sizeof one);
  if (n != sizeof one) {
    LOG(ERROR) << "eventloop wakeup writes " << n << " bytes instead of 8";
  }
}

void EventLoop::HandleWakeup() {
  uint64_t one = 1;
  ssize_t n = ::read(wakeup_fd_, &one, sizeof one);
  if (n != sizeof one) {
    LOG(ERROR) << "eventloop reads " << n << " bytes instead of 8";
  }
}

void EventLoop::DoPendingFunctors() {
  std::vector<Functor> functors;
  calling_pending_functors_ = true;

  {
    std::lock_guard<std::mutex> lock(_mutex);
    functors.swap(pending_functors_);
  }

  for (const Functor& functor : functors) {
    functor();
  }
  calling_pending_functors_ = false;
}

void EventLoop::PrintActiveEvents() {
  for (Event* ev : active_events_) {
    LOG(TRACE) << "{" << ev->ReventsToString() << "} ";
  }
}

void* EventLoop::Malloc(size_t len) { return mem_pool_->Malloc(len); }

void EventLoop::Free(void* ptr) { mem_pool_->Free(ptr); }

}  // namespace MSF