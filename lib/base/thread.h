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
#ifndef BASE_THREAD_H_
#define BASE_THREAD_H_

#include <pthread.h>  // std::thread
#include <sys/syscall.h>

#include <atomic>  // std::atomic_flag
#include <condition_variable>
#include <functional>
#include <iostream>
#include <thread>  // std::thread

#include "latch.h"
#include "gcc_attr.h"

using namespace MSF;

namespace MSF {

enum ThreadPriority {
#if defined(WIN32) || defined(WIN64)
  kLowPriority = THREAD_PRIORITY_BELOW_NORMAL,
  kNormalPriority = THREAD_PRIORITY_NORMAL,
  kHighPriority = THREAD_PRIORITY_ABOVE_NORMAL,
  kHighestPriority = THREAD_PRIORITY_HIGHEST,
  kRealtimePriority = THREAD_PRIORITY_TIME_CRITICAL
#else
  kLowPriority = 1,
  kNormalPriority = 2,
  kHighPriority = 3,
  kHighestPriority = 4,
  kRealtimePriority = 5
#endif
};

namespace CurrentThread {
int tid();
const char* tidString();
int tidStringLength();
const char* name();

bool isMainThread();
void sleepUsec(int64_t usec);

}  // namespace CurrentThread

typedef std::function<void()> ThreadFunc;

int pthreadSpawn(pthread_t* tid, void* (*pfn)(void*), void* arg);

// Represents a simple worker thread.  The implementation must be assumed
// to be single threaded, meaning that all methods of the class, must be
// called from the same thread, including instantiation.
class Thread {
 public:
  explicit Thread(ThreadFunc func, const std::string& name = std::string(),
                  ThreadPriority priority = kNormalPriority);
  ~Thread();

  // Spawns a thread and tries to set thread priority according to the priority
  // from when CreateThread was called.
  void Start();
  bool IsRunning() const;
  bool IsCurrent() const;
  // Stops (joins) the spawned thread.
  void Stop();

  void start(const ThreadFunc& initFunc = ThreadFunc());
  void join();
  int kill(int signal);

  bool started() const { return started_; }
  // pthread_t pthreadId() const {
  //     std::thread::id this_id = std::this_thread::get_id();
  //     return this_id;
  // }
  pid_t tid() const { return tid_; }
  const std::string& name() const { return name_; }

  // static int numCreated() { return numCreated_.fetch_and(); }

  // Yield the current thread so another thread can be scheduled.
  static void YieldCurrentThread();

 private:
  void Run();
  bool SetPriority(ThreadPriority priority);
  void setDefaultName();
  // Sets the thread name visible to debuggers/tools. This has no effect
  // otherwise. This name pointer is not copied internally. Thus, it must stay
  // valid until the thread ends.
  void SetCurrentThreadName(const char* name);
  std::thread thread_;
  bool started_;
  const ThreadPriority priority_ = kNormalPriority;

#if defined(WIN32) || defined(WIN64)
  static DWORD WINAPI StartThread(void* param);
  HANDLE th_ = nullptr;
  DWORD thread_id_ = 0;
#else
  // static void* StartThread(void* param);
  pthread_t th_ = 0;
#endif  // defined(WEBRTC_WIN)
  pid_t tid_;

  ThreadFunc func_;
  std::string name_;

  CountDownLatch latch_;
  void* threadLoop(ThreadFunc initFunc);
};

class ThreadFactory {
 public:
  virtual ~ThreadFactory() = default;
  // virtual std::thread newThread(Func&& func) = 0;
};

}  // namespace MSF
#endif