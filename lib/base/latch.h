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
#ifndef BASE_COUNTDOWNLATCH_H_
#define BASE_COUNTDOWNLATCH_H_

#include <condition_variable>
#include <mutex>

#include "noncopyable.h"

using namespace MSF;

namespace MSF {

/**
 * C++并发编程——闭锁CountDownLatch实现
 * https://blog.csdn.net/KID_LWC/article/details/98269619
 * https://blog.csdn.net/zhangxiao93/article/details/72677207
 *
 * C++11 并发指南std::condition_variable详解
 * https://www.cnblogs.com/wangshaowei/p/9593201.html
 * https://www.jianshu.com/p/c1dfa1d40f53
 *
 * 使用条件变量condition_variable, 什么条件下会虚假唤醒？
 * https://segmentfault.com/q/1010000010421523/a-1020000010457503
 * */
class CountDownLatch : public Noncopyable {
 public:
  explicit CountDownLatch(int count);
  ~CountDownLatch() = default;
  void Wait();
  bool WaitFor(const uint32_t ts);
  bool WaitUntil();
  void CountDown();
  void set_count(const int count) { count_ = count; }
  int count() const;

 private:
  std::mutex mutex_;
  std::condition_variable condition_;
  int count_;
};

}  // namespace MSF
#endif