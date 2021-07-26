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
#include "thread_pool.h"

#include <assert.h>
#include <base/logging.h>

#include "exception.h"
#include "thread.h"

using namespace MSF;
using namespace MSF::CurrentThread;

namespace MSF {

ThreadPool::ThreadPool(const std::string& name)
    : name_(name),
      running_(false),
      max_queue_size_(0),
      mutex_(),
      not_empty_(),
      not_full_() {}

ThreadPool::~ThreadPool() {
  if (running_) {
    Stop();
  }
}

void ThreadPool::Start(int numThreads) {
  assert(threads_.empty());
  threads_.reserve(numThreads);
  running_ = true;
  ThreadOption option;
  for (int i = 0; i < numThreads; ++i) {
    option.set_name("th-" + std::to_string(i + 1));
    option.set_priority(kNormalPriority);
    option.set_stack_size(4 * 1024 * 1024);
    option.set_loop_cb(std::bind(&ThreadPool::RunInThread, this));
    threads_.emplace_back(new Thread(option));
    threads_[i]->Start();
  }

  if (numThreads == 0 && thread_init_cb_) {
    thread_init_cb_();
  }
  LOG(INFO) << "all thread init completed";
}

void ThreadPool::Stop() {
  /* 保证lock的保护范围在括号内激活所有的线程*/
  {
    std::lock_guard<std::mutex> lock(mutex_);
    running_ = false;
  }
  not_empty_.notify_all();
  for (auto& thr : threads_) {
    thr->Join();
  }
}

size_t ThreadPool::QueueSize() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return queue_.size();
}

// https://zhuanlan.zhihu.com/p/358592479
#if 0
// add new work item to the pool
template <class F, class... Args>
auto ThreadPool::EnterQueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type> {

  using return_type = typename std::result_of<F(Args...)>::type;

  auto task = std::make_shared<std::packaged_task<return_type()>>(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));

  std::future<return_type> res = task->get_future();
  {
    std::unique_lock<std::mutex> lock(queue_mutex);

    // don't allow enqueueing after stopping the pool
    if (stop) throw std::runtime_error("enqueue on stopped ThreadPool");

    tasks.emplace([task]() { (*task)(); });
  }
  condition.notify_one();
  return res;
}
#endif

void ThreadPool::AddTask(ThreadTask task) {
  if (threads_.empty()) {
    task();
  } else {
    std::unique_lock<std::mutex> lock(mutex_);
    while (IsFull()) {
      not_full_.wait(lock);
    }
    if (!running_) {
      LOG(ERROR) << "thread pool not running";
      lock.unlock();
      return;
    }
    queue_.push_back(std::move(task));
    lock.unlock();
    not_empty_.notify_one();
  }
}

ThreadPool::ThreadTask ThreadPool::GetTask() {
  std::unique_lock<std::mutex> lock(mutex_);
  // always use a while-loop, due to spurious wakeup
  while (queue_.empty() && running_) {
    not_empty_.wait(lock);
  }

  ThreadTask task = queue_.front();
  queue_.pop_front();
  lock.unlock();
  if (max_queue_size_ > 0) {
    not_full_.notify_one();
  }
  return task;
}

bool ThreadPool::IsFull() const {
  return max_queue_size_ > 0 && queue_.size() >= max_queue_size_;
}

void ThreadPool::RunInThread() {
  try {
    if (thread_init_cb_) {
      thread_init_cb_();
    }
    while (running_) {
      ThreadTask task(GetTask());
      if (task) {
        task();
      }
    }
  }
  catch (const MSF::Exception& ex) {
    LOG(FATAL) << "exception caught in ThreadPool "
               << ",reason: " << ex.what()
               << "stack trace: " << ex.stackTrace();
    abort();
  }
  catch (const std::exception& ex) {
    LOG(FATAL) << "exception caught in ThreadPool "
               << ",reason: " << ex.what();
    abort();
  }
  catch (...) {
    LOG(FATAL) << "unknown exception caught in ThreadPool ";
    throw;  // rethrow
  }
}

}  // namespace MSF