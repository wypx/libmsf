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

#include <base/logging.h>

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
  if (unlikely(!started_)) {
    LOG(FATAL) << "event stack not start.";
    return nullptr;
  }

  if (thread_args_.size() == 0) {
    LOG(INFO) << "event stack return baseloop.";
    return &base_loop_;
  }

  std::default_random_engine e;
  std::uniform_int_distribution<unsigned> u(
      0, thread_args_.size() - 1);  //随机数分布对象
  return thread_args_[u(e)].loop_;
}

EventLoop *EventStack::GetHashLoop() {
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

void EventStack::StackLoop(StackArg *arg) {
  // not 100% race-free, eg. threadFunc could be running callback_.
  if (init_cb_) {
    init_cb_(arg->loop_);
  }
  LOG(INFO) << "============>";
  arg->loop_->EnterLoop();
  LOG(INFO) << "<============>";

  // still a tiny chance to call destructed object, if threadFunc exits just
  // now. but when EventLoopThread destructs, usually programming is exiting
  // anyway.
  arg->loop_->QuitLoop();
  arg->thread_->Join();
}

void EventStack::SetStackArgs(const std::vector<StackArg> &thread_arg) {
  std::move(thread_arg.begin(), thread_arg.end(),
            std::back_inserter(thread_args_));
}

void EventStack::StartStack() {
  base_loop_.AssertInLoopThread();
  if (unlikely(started_)) {
    LOG(FATAL) << "event loop thread pool is started.";
    return;
  }
  started_ = true;

  ThreadOption option;

  for (auto &th : thread_args_) {
    option.set_name(th.name_);
    option.set_priority(kNormalPriority);
    option.set_stack_size(1024 * 1024);
    option.set_loop_cb(std::bind(&EventStack::StackLoop, this, &th));
    th.thread_.reset(new Thread(option));
    th.thread_->Start([this, &th] { th.loop_ = new EventLoop(); });
  }
  LOG(INFO) << "start all " << thread_args_.size() << " thread success.";
}

bool EventStack::StartMain(const ThreadInitCallback &init_cb) {
  if (thread_args_.size() == 0) {
    if (init_cb) {
      init_cb(&base_loop_);
    }
  }

  base_loop_.EnterLoop();
  return true;
}

}  // namespace MSF
