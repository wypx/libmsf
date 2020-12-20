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
#ifndef BASE_BLOCKQUEUE_H_
#define BASE_BLOCKQUEUE_H_

#include <condition_variable>
#include <deque>
#include <mutex>

namespace MSF {

// https://blog.csdn.net/sai_j/article/details/83003421

template <typename T>
class BlockingQueue {
 public:
  BlockingQueue() : mutex_(), cond_(), queue_() {}

  BlockingQueue(const BlockingQueue&) = delete;
  BlockingQueue& operator=(const BlockingQueue&) = delete;

  void put(const T& x) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back(x);
    cond_.notify_one();
  }

  void put(T&& x) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back(x);
    cond_.notify_one();
  }

  T take() {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this] { return !queue_.empty(); });
    assert(!queue_.empty());
    T front(std::move(queue_.front()));
    queue_.pop_front();
    return front;
  }

  size_t size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
  }

 private:
  mutable std::mutex mutex_;
  std::condition_variable cond_;
  std::deque<T> queue_;
};

}  // namespace MSF
#endif