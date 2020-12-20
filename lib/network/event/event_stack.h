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
using namespace MSF::THREAD;

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

class EventStack : Noncopyable {
 public:
  typedef std::function<void(EventLoop *)> ThreadInitCallback;

  explicit EventStack();
  ~EventStack();
  void setThreadArgs(const std::vector<ThreadArg> &threadArgs);
  bool startThreads(const std::vector<ThreadArg> &threadArgs);
  bool start(const ThreadInitCallback &cb = ThreadInitCallback());

  // valid after calling start()
  /// round-robin
  EventLoop *getOneLoop();
  EventLoop *getHashLoop();
  EventLoop *getBaseLoop();
  EventLoop *GetFixedLoop(uint32_t idx);
  std::vector<EventLoop *> getAllLoops();

  bool started() const { return started_; }

 private:
  void startLoop(ThreadArg *arg);

 private:
  EventLoop baseLoop_;
  bool exiting_;
  bool started_;
  uint32_t next_;
  std::mutex mutex_;
  std::condition_variable cond_;

  ThreadInitCallback initCb_;

  std::vector<ThreadArg> threadArgs_;
};

}  // namespace MSF
#endif