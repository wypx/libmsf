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
#ifndef EVENT_EVENTSTACK_H_
#define EVENT_EVENTSTACK_H_

#include <condition_variable>
#include <mutex>

#include "event_loop.h"
#include "noncopyable.h"
#include "thread.h"

using namespace MSF;

namespace MSF {

struct ThreadArg {
  std::string name_;
  uint32_t options_;
  uint32_t flags_;

  EventLoop *loop_;
  Thread *thread_;

  ThreadArg() = default;
  ThreadArg(const std::string &name) : name_(name), options_(0), flags_(0) {}
  ThreadArg(const std::string &name, const uint32_t options)
      : name_(name), options_(options), flags_(0) {}
  ThreadArg(const std::string &name, const uint32_t options,
            const uint32_t flags)
      : name_(name), options_(options), flags_(flags) {}
};

class EventStack : noncopyable {
 public:
  typedef std::function<void(EventLoop *)> ThreadInitCallback;

  explicit EventStack();
  ~EventStack();
  void SetThreadArgs(const std::vector<ThreadArg> &threadArgs);
  bool StartThreads(const std::vector<ThreadArg> &threadArgs);
  bool Start(const ThreadInitCallback &cb = ThreadInitCallback());

  // valid after calling start()
  /// round-robin
  EventLoop *GetOneLoop();
  EventLoop *GetHashLoop();
  EventLoop *GetBaseLoop();
  EventLoop *GetFixedLoop(uint32_t idx);
  std::vector<EventLoop *> GetAllLoops();

  bool Started() const { return started_; }

 private:
  void StartLoop(ThreadArg *arg);

 private:
  EventLoop base_loop_;
  bool exiting_;
  bool started_;
  uint32_t next_;
  std::mutex mutex_;
  std::condition_variable cond_;

  ThreadInitCallback init_cb_;

  std::vector<ThreadArg> thread_args_;
};

}  // namespace MSF
#endif