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
#ifndef LIB_THREADPOOL_H_
#define LIB_THREADPOOL_H_

#include <unistd.h>

#include <deque>
#include <mutex>
#include <vector>

#include "noncopyable.h"
#include "thread.h"

using namespace MSF;

namespace MSF {

/**
 * C++封装POSIX 线程库（六）线程池
 * https://blog.csdn.net/zhangxiao93/article/details/72677704
 * https://blog.csdn.net/zhangxiao93/article/details/52027302
 *
 * 目前是一个无限长度的Queue的版本
 * TODO:
 * 1 改成有限长度的Queue
 * 2 增加一个future/promise的选项
 * */

class ThreadPool final : public noncopyable {
 public:
  typedef std::function<void()> ThreadTask;

  explicit ThreadPool(const std::string& name = std::string("ThreadPool"));
  virtual ~ThreadPool();

  // Must be called before start().
  void SetMaxQueueSize(int maxSize) { max_queue_size_ = maxSize; }
  void SetThreadInitCallback(const ThreadTask& cb) { thread_init_cb_ = cb; }

  void Start(int numThreads);
  void Stop();

  const std::string& name() const { return name_; }
  size_t QueueSize() const;

  template <class F, class... Args>
  auto EnterQueue(F&& f, Args&&... args);

  // Could block if maxQueueSize > 0
  // There is no move-only version of std::function in C++ as of C++14.
  // So we don't need to overload a const& and an && versions
  // as we do in (Bounded)BlockingQueue.
  // https://stackoverflow.com/a/25408989
  void AddTask(ThreadTask f);

  bool IsRunning() const { return running_; }

 private:
  bool IsFull() const;
  void RunInThread();
  ThreadTask GetTask();

  std::string name_;
  volatile bool running_;

  // the task queue
  std::deque<ThreadTask> queue_;
  size_t max_queue_size_;

  // synchronization
  mutable std::mutex mutex_;
  std::condition_variable not_empty_;
  std::condition_variable not_full_;

  ThreadTask thread_init_cb_;
  // need to keep track of threads so we can join them
  std::vector<std::unique_ptr<Thread>> threads_;

  std::atomic<uint32_t> max_threads_;
  std::atomic<uint32_t> current_threads_;
  std::atomic<uint32_t> max_idle_threads_;
  static thread_local bool working_;
};

}  // namespace MSF
#endif