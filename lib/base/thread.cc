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
#include "thread.h"

#include <assert.h>
#include <butil/logging.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#define gettid() syscall(__NR_gettid)

#include <iostream>
#include <type_traits>

#include "exception.h"

using namespace MSF;

namespace MSF {
namespace CurrentThread {

/**
 * https://www.jianshu.com/p/495ea7ce649b?utm_source=oschina-app
 * https://blog.csdn.net/u012528000/article/details/81146176
 * https://blog.csdn.net/qq_30071413/article/details/95205072
 **/
__thread uint64_t t_cachedTid = 0;
__thread char t_tidString[32];
__thread int t_tidStringLength = 6;
__thread const char* t_threadName = "unknown";
static_assert(std::is_same<int, pid_t>::value, "pid_t should be int");

/**
 * Get a process-specific identifier for the current thread.
 *
 * The return value will uniquely identify the thread within the current
 * process.
 *
 * Note that the return value does not necessarily correspond to an operating
 * system thread ID.  The return value is also only unique within the current
 * process: getCurrentThreadID() may return the same value for two concurrently
 * running threads in separate processes.
 *
 * The thread ID may be reused once the thread it corresponds to has been
 * joined.
 */
inline uint64_t getCurrentThreadID() {
#if __APPLE__
  return uint64_t(::pthread_mach_thread_np(::pthread_self()));
#elif defined(_WIN32)
  return uint64_t(GetCurrentThreadId());
#else
  return uint64_t(pthread_self());
#endif
}

/* pthread_self()
 * get unique thread id in one process, but not unique in system.
 * so to get unique tis, using SYS_gettid.
 */
inline uint64_t getOSThreadID() {
// Pthreads doesn't have the concept of a thread ID, so we have to reach down
// into the kernel.
#if __APPLE__
  uint64_t tid;
  ::pthread_threadid_np(nullptr, &tid);
  return tid;
#elif defined(WIN32)
  return uint64_t(GetCurrentThreadId());
#else
  return uint64_t(syscall(__NR_gettid));
// return uint64_t(syscall(SYS_gettid));
#endif
}

void cacheTid() {
  if (t_cachedTid == 0) {
    t_cachedTid = getOSThreadID();
    t_tidStringLength =
        snprintf(t_tidString, sizeof t_tidString, "%5ld ", t_cachedTid);
  }
}
int tid() {
  if (unlikely(t_cachedTid == 0)) {
    cacheTid();
  }
  return t_cachedTid;
}

const char* tidString()  // for logging
{
  return t_tidString;
}

int tidStringLength()  // for logging
{
  return t_tidStringLength;
}

const char* name() { return t_threadName; }

void sleepUsec(int64_t usec) {
  // struct timespec ts = { 0, 0 };
  // ts.tv_sec = static_cast<time_t>(usec / Timestamp::kMicroSecondsPerSecond);
  // ts.tv_nsec = static_cast<long>(usec % Timestamp::kMicroSecondsPerSecond *
  // 1000);
  // ::nanosleep(&ts, NULL);
}

}  // namespace CurrentThread

// pthread_cond_init
// pthread_cond_destrory
// pthread_cond_wait
// pthread_cond_timewait
// pthread_cond_signal
// pthread_cond_broadcast

// pthread_rwlock_init
// pthread_rwlock_destroy
// pthread_rwlock_rdlock/tryrdlock
// pthread_rwlock_wrlock/trywrlock
// pthread_rwlock_iunlock
// pthread_rwlock_equal
// pthread_rwlock_self

// pthread_cleanup_push
// pthread_cleanup_pop
// pthread_mutex_lock/unlock
// pthread_mutex_trylock
// pthread_mutex_init
// pthread_mutex_destroy

// pthread_set/get_cancelstate
// pthread_testcancle
// pthread_setcacltype

int pthreadSpawn(pthread_t* tid, void* (*pfn)(void*), void* arg) {
  int rc;
  pthread_attr_t thread_attr;

#ifndef WIN32
  sigset_t signal_mask;
  ::sigemptyset(&signal_mask);
  ::sigaddset(&signal_mask, SIGPIPE);
  rc = ::pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);
  if (rc != 0) {
    LOG(ERROR) << "Block sigpipe error.";
  }
#endif

  ::pthread_attr_init(&thread_attr);
  ::pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
  if (::pthread_create(tid, &thread_attr, pfn, arg) < 0) {
    LOG(ERROR) << "Do pthread_create errno: " << errno;
    return -1;
  }

  ::pthread_attr_destroy(&thread_attr);

  return 0;
}

#if !defined(WIN32) && !defined(WIN64)
struct ThreadAttributes {
  ThreadAttributes() { pthread_attr_init(&attr); }
  ~ThreadAttributes() { pthread_attr_destroy(&attr); }
  pthread_attr_t* operator&() { return &attr; }
  pthread_attr_t attr;
};
#endif  // defined(WIN)

class ThreadNameInitializer {
 public:
  static void afterFork() {
    CurrentThread::t_cachedTid = 0;
    CurrentThread::t_threadName = "main";
    CurrentThread::tid();
    // no need to call pthread_atfork(NULL, NULL, &afterFork);
  }
  ThreadNameInitializer() {
    CurrentThread::t_threadName = "main";
    CurrentThread::tid();
    pthread_atfork(NULL, NULL, &afterFork);
  }
};

ThreadNameInitializer init;

Thread::Thread(ThreadFunc func, const std::string& name,
               ThreadPriority priority)
    : started_(false),
      priority_(priority),
      tid_(0),
      func_(std::move(func)),
      name_(name),
      latch_(1) {
  setDefaultName();
}

/**
 * use in pthread
 * pthread_t _pthreadId;
 * pthread_detach(_pthreadId);
 * */
Thread::~Thread() {
  if (started_ && !thread_.joinable()) {
    thread_.detach();
  }
  started_ = false;
}

void* Thread::threadLoop(ThreadFunc initFunc) {
  if (initFunc) {
    initFunc();
  }
  tid_ = CurrentThread::tid();
  CurrentThread::t_threadName = name_.c_str();
  LOG(INFO) << "Thread name: " << CurrentThread::t_threadName
            << " name:" << name_;
  SetCurrentThreadName(CurrentThread::t_threadName);
  SetPriority(priority_);

  latch_.CountDown();

  try {
    func_();
    started_ = false;
    CurrentThread::t_threadName = "finished";
  }
  catch (const Exception& ex) {
    CurrentThread::t_threadName = "crashed";
    LOG(FATAL) << "exception caught in ThreadPool " << name_.c_str()
               << ",reason: " << ex.what()
               << "stack trace: " << ex.stackTrace();
    abort();
  }
  catch (const std::exception& ex) {
    CurrentThread::t_threadName = "crashed";
    LOG(FATAL) << "exception caught in ThreadPool " << name_.c_str()
               << ",reason: " << ex.what();
    abort();
  }
  catch (...) {
    CurrentThread::t_threadName = "crashed";
    LOG(FATAL) << "unknown exception caught in ThreadPool " << name_.c_str();
    throw;  // rethrow
  }
  return nullptr;
}

void Thread::SetCurrentThreadName(const char* name) {
#if defined(WIN32) || defined(WIN64)
// For details see:
// https://docs.microsoft.com/en-us/visualstudio/debugger/how-to-set-a-thread-name-in-native-code
#pragma pack(push, 8)
  struct {
    DWORD dwType;
    LPCSTR szName;
    DWORD dwThreadID;
    DWORD dwFlags;
  } threadname_info = {0x1000, name, static_cast<DWORD>(-1), 0};
#pragma pack(pop)

#pragma warning(push)
#pragma warning(disable : 6320 6322)
  __try {
    ::RaiseException(0x406D1388, 0, sizeof(threadname_info) / sizeof(ULONG_PTR),
                     reinterpret_cast<ULONG_PTR*>(&threadname_info));
  }
  __except(EXCEPTION_EXECUTE_HANDLER) {  // NOLINT
  }
#pragma warning(pop)
#elif defined(__linux__)
  ::prctl(PR_SET_NAME, reinterpret_cast<unsigned long>(name));  // NOLINT
#elif defined(__APPLE__)
  ::pthread_setname_np(name);
#endif
}

/*
 * 当前进程仍然处于可执行状态,但暂时"让出"处理器,
 * 使得处理器优先调度其他可执行状态的进程, 这样在
 * 进程被内核再次调度时,在for循环代码中可以期望其他进程释放锁
 * 注意,不同的内核版本对于sched_yield系统调用的实现可能是不同的,
 * 但它们的目的都是暂时"让出"处理器*/
void Thread::YieldCurrentThread() {
#if defined(WIN32) || defined(WIN64)
  ::SwitchToThread();
#elif defined(__linux__)
  ::sched_yield();
#else
  ::usleep(1);
#endif
}

bool Thread::SetPriority(ThreadPriority priority) {
#if defined(WIN32) || defined(WIN64)
  return ::SetThreadPriority(th_, priority) != FALSE;
#else
  const int policy = SCHED_FIFO;
  const int min_prio = ::sched_get_priority_min(policy);
  const int max_prio = ::sched_get_priority_max(policy);
  if (min_prio == -1 || max_prio == -1) {
    return false;
  }

  if (max_prio - min_prio <= 2) return false;

  // Convert webrtc priority to system priorities:
  sched_param param;
  const int top_prio = max_prio - 1;
  const int low_prio = min_prio + 1;
  switch (priority) {
    case kLowPriority:
      param.sched_priority = low_prio;
      break;
    case kNormalPriority:
      // The -1 ensures that the kHighPriority is always greater or equal to
      // kNormalPriority.
      param.sched_priority = (low_prio + top_prio - 1) / 2;
      break;
    case kHighPriority:
      param.sched_priority = std::max(top_prio - 2, low_prio);
      break;
    case kHighestPriority:
      param.sched_priority = std::max(top_prio - 1, low_prio);
      break;
    case kRealtimePriority:
      param.sched_priority = top_prio;
      break;
  }
  return ::pthread_setschedparam(th_, policy, &param) == 0;
#endif
}

void Thread::setDefaultName() {
  // int num = _numCreated.fetch_add(1);
  if (name_.empty()) {
    name_ = "Thread_" + std::to_string(gettid());
  }
}

void Thread::Start() {
#if defined(WIN32) || defined(WIN64)
  // See bug 2902 for background on STACK_SIZE_PARAM_IS_A_RESERVATION.
  // Set the reserved stack stack size to 1M, which is the default on Windows
  // and Linux.
  th_ = ::CreateThread(nullptr, 1024 * 1024, &StartThread, this,
                       STACK_SIZE_PARAM_IS_A_RESERVATION, &thread_id_);
  // assert(th_) << "CreateThread failed";
  assert(thread_id_);
#else
  ThreadAttributes attr;
  // Set the stack stack size to 1M.
  ::pthread_attr_setstacksize(&attr, 1024 * 1024);
// assert(0 == pthread_create(&th_, &attr, &StartThread, this));

// pthread_attr_getstack
// pthread_attr_setstack
// pthread_attr_get/setstacksize 默认pagesize
// pthread_attr_get/setgardsize
// pthread_attr_get/setconcureny(int level) 提高希望并发度
#endif
}

bool Thread::IsRunning() const {
#if defined(WIN32)
  return th_ != nullptr;
#else
  return th_ != 0;
#endif
}

bool Thread::IsCurrent() const { return (::pthread_self() == th_); }

void Thread::Stop() {
  if (!IsRunning()) return;

#if defined(WIN32)
  WaitForSingleObject(th_, INFINITE);
  CloseHandle(th_);
  th_ = nullptr;
  thread_id_ = 0;
#else
  assert(0 == ::pthread_join(th_, nullptr));
  th_ = 0;
#endif
}

/**
 * pthread_t _pthreadId;
 * pthreadSpawn(&_pthreadId, startThread, data)
 * */
void Thread::start(const ThreadFunc& initFunc) {
  assert(!started_);
  started_ = true;

  try {
    thread_ = std::thread(std::bind(&Thread::threadLoop, this, initFunc));
    latch_.Wait();
  }
  catch (const Exception& ex) {
    CurrentThread::t_threadName = "crashed";
    LOG(FATAL) << "Fail to create thread ThreadPool " << name_.c_str()
               << ",reason: " << ex.what()
               << "stack trace: " << ex.stackTrace();
    started_ = false;
  }
  catch (const std::exception& ex) {
    CurrentThread::t_threadName = "crashed";
    LOG(FATAL) << "Fail to create thread ThreadPool " << name_.c_str()
               << ",reason: " << ex.what();
    started_ = false;
  }
  catch (...) {
    CurrentThread::t_threadName = "crashed";
    LOG(FATAL) << "Fail to create thread ThreadPool, unkown exception "
               << name_.c_str();
    started_ = false;
    throw;  // rethrow
  }
}

/**
 * used in pthread
 * pthread_t _pthreadId;
 * pthread_join(_pthreadId, NULL)
 * */
void Thread::join() {
  if (!started_) {
    LOG(ERROR) << "Thread already started";
    return;
  }

  if (!thread_.joinable()) {
    LOG(WARNING) << "Thread not allow to join";
    return;
  }
  thread_.join();
}

int Thread::kill(int signal) {
  if (th_)
    return ::pthread_kill(thread_.native_handle(), signal);
  else
    return -EINVAL;
}

}  // namespace MSF