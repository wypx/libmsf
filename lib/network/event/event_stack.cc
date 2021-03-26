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
#include "event_stack.h"

#include <butil/logging.h>

#include <random>

#include "signal_manager.h"

namespace MSF {

EventStack::EventStack() : base_loop_(EventLoop()), started_(false) {
  InitSigHandler();
}

EventStack::~EventStack() {
  // Don't delete loop, it's stack variable
}

EventLoop *EventStack::GetOneLoop() {
  // base_loop_.assertInLoopThread();
  if (unlikely(!started_)) {
    LOG(FATAL) << "event loop thread pool not start.";
    return nullptr;
  }

  if (thread_args_.size() == 0) {
    LOG(INFO) << "event loop return baseloop.";
    return &base_loop_;
  }

  std::default_random_engine e;
  std::uniform_int_distribution<unsigned> u(
      0, thread_args_.size() - 1);  //随机数分布对象
  return thread_args_[u(e)].loop_;
}

EventLoop *EventStack::GetHashLoop() {
  // base_loop_.assertInLoopThread();
  if (unlikely(!started_)) {
    LOG(FATAL) << "event loop thread pool not start.";
    return nullptr;
  }

  if (thread_args_.size() == 0) {
    LOG(INFO) << "event loop return baseloop.";
    return &base_loop_;
  }
  return thread_args_[(next_++) % thread_args_.size()].loop_;
}

EventLoop *EventStack::GetBaseLoop() { return &base_loop_; }

EventLoop *EventStack::GetFixedLoop(uint32_t idx) {
  if (idx < thread_args_.size()) {
    return thread_args_[idx].loop_;
  }
  return nullptr;
}
std::vector<EventLoop *> EventStack::GetAllLoops() {
  std::vector<EventLoop *> loops;
  for (const auto &th : thread_args_) {
    loops.push_back(th.loop_);
  }
  return loops;
}

void EventStack::StartLoop(ThreadArg *arg) {
  // not 100% race-free, eg. threadFunc could be running callback_.
  if (arg->loop_ != nullptr) {
    if (init_cb_) {
      init_cb_(arg->loop_);
    }
    arg->loop_->EnterLoop();

    // still a tiny chance to call destructed object, if threadFunc exits just
    // now. but when EventLoopThread destructs, usually programming is exiting
    // anyway.
    arg->loop_->QuitLoop();
    arg->thread_->join();
    arg->loop_ = nullptr;
  } else {
    LOG(FATAL) << "event loop pointer is invalid";
  }
}

void EventStack::SetThreadArgs(const std::vector<ThreadArg> &thread_arg) {
  std::move(thread_arg.begin(), thread_arg.end(),
            std::back_inserter(thread_args_));
}

bool EventStack::StartThreads(const std::vector<ThreadArg> &thread_args) {
  if (unlikely(started_)) {
    LOG(FATAL) << "event loop thread pool is started.";
    return false;
  }
  // base_loop_->assertInLoopThread();
  started_ = true;

  std::move(thread_args.begin(), thread_args.end(),
            std::back_inserter(thread_args_));

  for (auto &th : thread_args_) {
    th.thread_ =
        new Thread(std::bind(&EventStack::StartLoop, this, &th), th.name_);
    if (th.thread_ == nullptr) {
      LOG(FATAL) << "alloc event thread failed, errno:" << errno;
      return false;
    }
    th.thread_->start([this, &th]() {
      /* Pre init function before thread loop */
      th.loop_ = new EventLoop();
      if (th.loop_ == nullptr) {
        LOG(FATAL) << "alloc event loop failed, errno:" << errno;
        return;
      }
    });
  }
  LOG(INFO) << "start all " << thread_args_.size() << " thread success.";
  return true;
}

bool EventStack::Start(const ThreadInitCallback &cb) {
  if (thread_args_.size() == 0) {
    if (cb) {
      cb(&base_loop_);
    }
  }

  base_loop_.EnterLoop();

  return true;
}

}  // namespace MSF
