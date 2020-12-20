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
#ifndef __MSF_THREADPOOL_H__
#define __MSF_THREADPOOL_H__

#include <unistd.h>

#include <deque>
#include <mutex>
#include <vector>

#include "noncopyable.h"
#include "thread.h"

using namespace MSF::THREAD;
using namespace MSF;

namespace MSF {

/**
 * C++封装POSIX 线程库（六）线程池
 * https://blog.csdn.net/zhangxiao93/article/details/72677704
 * https://blog.csdn.net/zhangxiao93/article/details/52027302
 *
 * */

class ThreadPool final : public Noncopyable {
 public:
  typedef std::function<void()> ThreadTask;

  explicit ThreadPool(const std::string& name = std::string("ThreadPool"));
  virtual ~ThreadPool();

  // Must be called before start().
  void setMaxQueueSize(int maxSize) { _maxQueueSize = maxSize; }
  void setThreadInitCallback(const ThreadTask& cb) { _threadInitCallback = cb; }

  void start(int numThreads);
  void stop();

  const std::string& name() const { return _name; }
  size_t queueSize() const;

  // Could block if maxQueueSize > 0
  // There is no move-only version of std::function in C++ as of C++14.
  // So we don't need to overload a const& and an && versions
  // as we do in (Bounded)BlockingQueue.
  // https://stackoverflow.com/a/25408989
  void addTask(ThreadTask f);

  bool isRunning() const { return _running; }

 private:
  bool isFull() const;
  void runInThread();
  ThreadTask getTask();

  std::string _name;
  volatile bool _running;

  mutable std::mutex _mutex;
  std::deque<ThreadTask> _queue;
  size_t _maxQueueSize;
  std::condition_variable _notEmpty;
  std::condition_variable _notFull;

  ThreadTask _threadInitCallback;
  std::vector<std::unique_ptr<Thread>> _threads;

  std::atomic<uint32_t> _maxThreads;
  std::atomic<uint32_t> _currentThreads;
  std::atomic<uint32_t> _maxIdleThreads;
  static thread_local bool _working;
};

}  // namespace MSF
#endif