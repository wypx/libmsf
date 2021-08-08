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

#include "gcc_attr.h"
#include "latch.h"

using namespace MSF;

namespace MSF {

namespace CurrentThread {
int tid();
const char* tidString();
int tidStringLength();
const char* name();

bool isMainThread();
void sleepUsec(int64_t usec);

}  // namespace CurrentThread

typedef std::function<void()> ThreadCallback;

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

struct ThreadOption {
  std::string name_;
  ThreadCallback loop_cb_;
  ThreadPriority priority_ = kNormalPriority;
  uint32_t stack_size_ = 1024 * 1024;
  bool is_detach_ = false;

  ThreadOption() = default;
  void set_name(const std::string& name) noexcept { name_ = name; }
  const std::string& name() const { return name_; }
  void set_loop_cb(const ThreadCallback& loop_cb) noexcept {
    loop_cb_ = loop_cb;
  }
  const ThreadCallback& loop_cb() { return loop_cb_; }
  void set_priority(ThreadPriority priority) noexcept { priority_ = priority; }
  ThreadPriority priority() const { return priority_; }
  void set_stack_size(uint32_t stack_size = 1024 * 1024) noexcept {
    stack_size_ = stack_size;
  }
  uint32_t stack_size() const { return stack_size_; }
  void set_is_detach(bool is_detach) noexcept { is_detach_ = is_detach; }
  bool is_detach() const { return is_detach_; }
};

// Represents a simple worker thread.  The implementation must be assumed
// to be single threaded, meaning that all methods of the class, must be
// called from the same thread, including instantiation.
class Thread {
 public:
  explicit Thread(const ThreadOption& option);
  ~Thread();

  // Spawns a thread and tries to set thread priority according to the priority
  // from when CreateThread was called.
  void Start(const ThreadCallback& init_cb = ThreadCallback());
  bool IsRunning() const;
  bool IsCurrent() const;
  // Stops (joins) the spawned thread.
  void Stop();

  void Join();
  void Detach();
  int Kill(int signal);

  bool started() const { return started_; }
  // pthread_t pthreadId() const {
  //     std::thread::id this_id = std::this_thread::get_id();
  //     return this_id;
  // }
  pid_t tid() const { return tid_; }
  const std::string& name() const { return option_.name(); }

  // static int numCreated() { return numCreated_.fetch_and(); }

  // Yield the current thread so another thread can be scheduled.
  static void YieldCurrentThread();

 private:
  bool SetPriority(ThreadPriority priority);
  void InternalInit();
  void Run();
  bool started_ = false;

  std::thread thread_;
#if defined(WIN32) || defined(WIN64)
  static DWORD WINAPI StartThread(void* param);
  HANDLE th_ = nullptr;
  DWORD thread_id_ = 0;
#else
  // static void* StartThread(void* param);
  pthread_t th_ = 0;
#endif  // defined(WEBRTC_WIN)
  pid_t tid_ = 0;

  ThreadOption option_;

  void* ThreadLoop(const ThreadCallback& init_cb, CountDownLatch& latch);
};

class ThreadFactory {
 public:
  virtual ~ThreadFactory() = default;
  // virtual std::thread newThread(Func&& func) = 0;
};

}  // namespace MSF
#endif