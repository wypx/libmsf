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
#include "latch.h"

#include <base/logging.h>

using namespace MSF;

namespace MSF {

CountDownLatch::CountDownLatch(int count) : count_(count) {}

void CountDownLatch::Wait() {
  std::unique_lock<std::mutex> lock(mutex_);

  /**
   * 当前线程被阻塞 Unlock mu and wait to be notified
   * https://www.jianshu.com/p/c1dfa1d40f53
   * https://zh.cppreference.com/w/cpp/thread/condition_variable
   * //https://blog.csdn.net/business122/article/details/80881925
   * */
  condition_.wait(lock, [this]() { return (count_ == 0); });

  lock.unlock();
}

// http://www.voidcn.com/article/p-wonhtnlp-bsz.html
// https://blog.csdn.net/fengbingchun/article/details/73695596
// https://www.cnblogs.com/haippy/p/3252041.html
// https://www.2cto.com/kf/201506/411327.html
// wait_for: std::cv_status::timeout
bool CountDownLatch::WaitFor(const uint32_t ts) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (condition_.wait_for(lock, std::chrono::seconds(ts)) ==
      std::cv_status::timeout /*,
     [this]() { 
       LOG(INFO) << "count wait: " << count_;
       return (count_ == 0);
       }) == false*/) {
    // LOG(INFO) << "count timeout: " << count_;
    return false;
  }
  return true;
}

bool CountDownLatch::WaitUntil() {
  // return condition_.wait_until(std::chrono::steady_clock::now() + rel_time);
  return true;
}

void CountDownLatch::CountDown() {
  // LOG(INFO) << "count is: " << count_;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    --count_;
  }

  if (count_ == 0) {
    condition_.notify_all();
  }
}

int CountDownLatch::count() const {
  // std::lock_guard<std::mutex> lock(mutex_);
  return count_;
}

}  // namespace MSF