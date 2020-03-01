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
#ifndef __MSF_THREAD_H__
#define __MSF_THREAD_H__

#include <base/CountDownLatch.h>
#include <base/GccAttr.h>
#include <sys/syscall.h>

#include <atomic>  // std::atomic_flag
#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <thread>  // std::thread

using namespace MSF::BASE;

namespace MSF {

/*
 * 当前进程仍然处于可执行状态,但暂时"让出"处理器,
 * 使得处理器优先调度其他可执行状态的进程, 这样在
 * 进程被内核再次调度时,在for循环代码中可以期望其他进程释放锁
 * 注意,不同的内核版本对于sched_yield系统调用的实现可能是不同的,
 * 但它们的目的都是暂时"让出"处理器*/
#if (MSF_HAVE_SCHED_YIELD)
#define msf_sched_yield() sched_yield()
#else
#define msf_sched_yield() usleep(1)
#endif

namespace CurrentThread {
int tid();
const char* tidString();
int tidStringLength();
const char* name();

bool isMainThread();
void sleepUsec(int64_t usec);

}  // namespace CurrentThread

namespace THREAD {

typedef std::function<void()> ThreadFunc;

int pthreadSpawn(pthread_t* tid, void* (*pfn)(void*), void* arg);

class Thread {
 public:
  explicit Thread(ThreadFunc func, const std::string& name = std::string());
  ~Thread();

  void start(const ThreadFunc& initFunc = ThreadFunc());
  void join();

  bool started() const { return _started; }
  // pthread_t pthreadId() const {
  //     std::thread::id this_id = std::this_thread::get_id();
  //     return this_id;
  // }
  pid_t tid() const { return _tid; }
  const std::string& name() const { return _name; }

  // static int numCreated() { return numCreated_.fetch_and(); }

 private:
  void setDefaultName();
  std::thread _thread;

  bool _started;
  pid_t _tid;
  ThreadFunc _func;
  std::string _name;

  CountDownLatch _latch;
  void* threadLoop(ThreadFunc initFunc);
};

class ThreadFactory {
 public:
  virtual ~ThreadFactory() = default;
  // virtual std::thread newThread(Func&& func) = 0;
};

}  // namespace THREAD
}  // namespace MSF
#endif