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
#include <assert.h>
#include <base/Logger.h>
#include <base/Thread.h>
#include <event/EpollPoller.h>
#include <event/Event.h>
#include <event/EventLoop.h>
#include <sock/Socket.h>
#include <sys/eventfd.h>

#include <memory>

using namespace MSF::BASE;
using namespace MSF::SOCK;
using namespace MSF::EVENT;

namespace MSF {
namespace EVENT {

__thread EventLoop* t_loopInThisThread = nullptr;

const int kPollTimeMs = 10000;

EventLoop::EventLoop()
    : _looping(false),
      _quit(false),
      _eventHandling(false),
      _callingPendingFunctors(false),
      _iteration(0),
      _threadId(MSF::CurrentThread::tid()),
      _poller(new EPollPoller(this)),
      _currActiveEvent(nullptr) {
  MSF_INFO << "EventLoop created " << this << " in thread " << _threadId;
  if (t_loopInThisThread) {
    MSF_FATAL << "Another EventLoop " << t_loopInThisThread
              << " exists in this thread " << _threadId;
  } else {
    t_loopInThisThread = this;
  }

  // (0, EFD_NONBLOCK | EFD_CLOEXEC | EFD_SEMAPHORE);
  wakeupFd_ = CreateEventFd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  assert(wakeupFd_ > 0);
  wakeupEvent_ = std::make_unique<Event>(this, wakeupFd_);
  assert(wakeupEvent_);
  wakeupEvent_->setReadCallback(std::bind(&EventLoop::handleWakeup, this));
  // we are always reading the wakeupfd
  wakeupEvent_->enableReading();

  timer_ = std::make_unique<HeapTimer>(this, 1024);
  assert(timer_);
}

EventLoop::~EventLoop() {
  MSF_INFO << "EventLoop " << this << " of thread " << _threadId
           << " destructs in thread " << CurrentThread::tid();
  wakeupEvent_->disableAll();
  wakeupEvent_->remove();
  CloseSocket(wakeupFd_);
  t_loopInThisThread = NULL;

  //清理所有的事件?
}

void EventLoop::loop() {
  assert(!_looping);
  assertInLoopThread();

  uint32_t nextExpTime = 1;
  _looping = true;
  _quit = false;  // FIXME: what if someone calls quit() before loop() ?
  MSF_INFO << "EventLoop " << this << " start looping";

  while (!_quit) {
    _activeEvents.clear();
    int time = _poller->poll(nextExpTime, &_activeEvents);
    ++_iteration;
#if 0
    printActiveEvents();
#endif
    nextExpTime = timer_->timerInLoop();
    // MSF_WARN << "NextExpTime: " << nextExpTime;
    if (nextExpTime == 0) {
      nextExpTime = kPollTimeMs;
    }
    // TODO sort channel by priority
    _eventHandling = true;
    for (Event* ev : _activeEvents) {
      _currActiveEvent = ev;
      _currActiveEvent->handleEvent(time);
    }
    _currActiveEvent = NULL;
    _eventHandling = false;
    doPendingFunctors();
  }

  MSF_INFO << "EventLoop " << this << " stop looping";
  _looping = false;
}

void EventLoop::quit() {
  _quit = true;
  // There is a chance that loop() just executes while(!quit_) and exits,
  // then EventLoop destructs, then we are accessing an invalid object.
  // Can be fixed using mutex_ in both places.
  if (!isInLoopThread()) {
    wakeup();
  }
}

void EventLoop::runInLoop(Functor cb) {
  if (isInLoopThread()) {
    cb();
  } else {
    queueInLoop(std::move(cb));
  }
}

void EventLoop::queueInLoop(Functor cb) {
  {
    std::lock_guard<std::mutex> lock(_mutex);
    _pendingFunctors.push_back(std::move(cb));
  }

  if (!isInLoopThread() || _callingPendingFunctors) {
    wakeup();
  }
}

size_t EventLoop::queueSize() const {
  std::lock_guard<std::mutex> lock(_mutex);
  return _pendingFunctors.size();
}

TimerId EventLoop::runAt(const double time, const TimerCb& cb) {
  timer_->addTimer(cb, time, TIMER_ONESHOT);
  wakeup();
  return 0;
}

TimerId EventLoop::runAfter(const double delay, const TimerCb& cb) {
  timer_->addTimer(cb, delay, TIMER_ONESHOT);
  wakeup();
  return 0;
}

TimerId EventLoop::runEvery(const double interval, const TimerCb& cb) {
  timer_->addTimer(cb, interval, TIMER_PERSIST);
  wakeup();
  return 0;
}

void EventLoop::cancel(uint64_t timerId) {
  // return timerQueue_->cancel(timerId);
  // timer_->addTimer(cb, interval, TIMER_PERSIST);
  return;
}

void EventLoop::updateEvent(Event* ev) {
  assert(ev->ownerLoop() == this);
  // assertInLoopThread();
  _poller->updateEvent(ev);
}

void EventLoop::removeEvent(Event* ev) {
  assert(ev->ownerLoop() == this);
  assertInLoopThread();
  if (_eventHandling) {
    assert(_currActiveEvent == ev ||
           std::find(_activeEvents.begin(), _activeEvents.end(), ev) ==
               _activeEvents.end());
  }
  _poller->removeEvent(ev);
}

bool EventLoop::hasEvent(Event* ev) {
  assert(ev->ownerLoop() == this);
  assertInLoopThread();
  return _poller->hasEvent(ev);
}

bool EventLoop::isInLoopThread() const {
  return _threadId == MSF::CurrentThread::tid();
}

void EventLoop::abortNotInLoopThread() {
  MSF_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
            << " was created in threadId_ = " << _threadId
            << ", current thread id = " << CurrentThread::tid();
}

void EventLoop::wakeup() {
  uint64_t one = 1;
  ssize_t n = write(wakeupFd_, &one, sizeof one);
  if (n != sizeof one) {
    MSF_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
  }
}

void EventLoop::handleWakeup() {
  uint64_t one = 1;
  ssize_t n = read(wakeupFd_, &one, sizeof one);
  if (n != sizeof one) {
    MSF_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
  }
}

void EventLoop::doPendingFunctors() {
  std::vector<Functor> functors;
  _callingPendingFunctors = true;

  {
    std::lock_guard<std::mutex> lock(_mutex);
    functors.swap(_pendingFunctors);
  }

  for (const Functor& functor : functors) {
    functor();
  }
  _callingPendingFunctors = false;
}

void EventLoop::printActiveEvents() {
  for (Event* ev : _activeEvents) {
    MSF_DEBUG << "{" << ev->reventsToString() << "} ";
  }
}

}  // namespace EVENT
}  // namespace MSF