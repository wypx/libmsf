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
#include <errno.h>
#include <malloc.h>
#include <signal.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#define gettid() syscall(__NR_gettid)

#include <base/exception.h>
#include <base/logging.h>

#include <iostream>
#include <type_traits>

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

class ThreadNameInitializer {
 public:
  static void AfterFork() {
    CurrentThread::t_cachedTid = 0;
    CurrentThread::t_threadName = "main";
    CurrentThread::tid();
  }
  ThreadNameInitializer() {
    CurrentThread::t_threadName = "main";
    CurrentThread::tid();
    ::pthread_atfork(nullptr, nullptr, &AfterFork);
  }
};

ThreadNameInitializer init;

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

#if !defined(WIN32) && !defined(WIN64)
struct ThreadAttributes {
  ThreadAttributes() {
    ::pthread_attr_init(&attr_);
    ::pthread_attr_setdetachstate(&attr_, PTHREAD_CREATE_JOINABLE);
  }
  ~ThreadAttributes() { ::pthread_attr_destroy(&attr_); }
  pthread_attr_t* operator&() { return &attr_; }
  pthread_attr_t attr_;

  void SetThreadName(const char* name) {
#if defined(__linux__)
    ::prctl(PR_SET_NAME, reinterpret_cast<unsigned long>(name));  // NOLINT
#elif defined(__APPLE__)
    ::pthread_setname_np(name);
#endif
  }

#if SUPPORTS_PTHREAD_NAMING
#define PTHREAD_MAX_THREADNAME_LEN_INCLUDING_NULL_BYTE 16
  // Attempts to get the name from the operating system, returning true and
  // updating 'name' if successful. Note that during normal operation this
  // may fail, if the thread exits prior to the system call.
  bool GetNameFromOS(std::string& name) {
    // Verify that the name got written into the thread as expected.
    char buf[PTHREAD_MAX_THREADNAME_LEN_INCLUDING_NULL_BYTE];
    pthread_t thread_handle;
    const int get_name_rc = pthread_getname_np(thread_handle, buf, sizeof(buf));
    if (get_name_rc != 0) {
      return false;
    }
    name = buf;
    return true;
  }
#endif

  // Set the stack stack size to 1M
  bool SetStackSize(size_t stacksize = 1024 * 1024) {
    return (::pthread_attr_setstacksize(&attr_, stacksize) == 0);
  }
  size_t GetStackSize() const {
    size_t stacksize = 0;
    ::pthread_attr_getstacksize(&attr_, &stacksize);
    return stacksize;
  }

#if 0
/* On MacOS, threads other than the main thread are created with a reduced
 * stack size by default.  Adjust to RLIMIT_STACK aligned to the page size.
 *
 * On Linux, threads created by musl have a much smaller stack than threads
 * created by glibc (80 vs. 2048 or 4096 kB.)  Follow glibc for consistency.
 */
static size_t thread_stack_size(void) {
#if defined(__APPLE__) || defined(__linux__)
  struct rlimit lim;

  /* getrlimit() can fail on some aarch64 systems due to a glibc bug where
   * the system call wrapper invokes the wrong system call. Don't treat
   * that as fatal, just use the default stack size instead.
   */
  if (0 == ::getrlimit(RLIMIT_STACK, &lim) && lim.rlim_cur != RLIM_INFINITY) {
    /* pthread_attr_setstacksize() expects page-aligned values. */
    lim.rlim_cur -= lim.rlim_cur % (rlim_t) getpagesize();

    /* Musl's PTHREAD_STACK_MIN is 2 KB on all architectures, which is
     * too small to safely receive signals on.
     *
     * Musl's PTHREAD_STACK_MIN + MINSIGSTKSZ == 8192 on arm64 (which has
     * the largest MINSIGSTKSZ of the architectures that musl supports) so
     * let's use that as a lower bound.
     *
     * We use a hardcoded value because PTHREAD_STACK_MIN + MINSIGSTKSZ
     * is between 28 and 133 KB when compiling against glibc, depending
     * on the architecture.
     */
    if (lim.rlim_cur >= 8192)
      if (lim.rlim_cur >= PTHREAD_STACK_MIN)
        return lim.rlim_cur;
  }
#endif

#if !defined(__linux__)
  return 0;
#elif defined(__PPC__) || defined(__ppc__) || defined(__powerpc__)
  return 4 << 20;  /* glibc default. */
#else
  return 2 << 20;  /* glibc default. */
#endif
}
#endif

#if 0
  // pthread_attr_getstack
  // pthread_attr_setstack

  bool SetConcureny(int level) {
    ::pthread_attr_setconcureny(&attr_, &level);
  }
  int GetConcureny() {
    int level = 0;
    ::pthread_attr_getconcureny((&attr_, &level);
    return level;
  }
  bool SetGardSize() {
    ::pthread_attr_setgardsize(&attr_, &stacksize);
  }
  void GetGardSize() {
    pthread_attr_getgardsize(&attr_, &stacksize);
  }
#endif
  bool BlockSigPipe() {
#ifndef WIN32
    sigset_t signal_mask;
    ::sigemptyset(&signal_mask);
    ::sigaddset(&signal_mask, SIGPIPE);
    return (::pthread_sigmask(SIG_BLOCK, &signal_mask, nullptr) == 0);
#else
    return true;
#endif
  }
  bool SpawnThread(pthread_t* tid, void* (*pfn)(void*), void* arg) {
    return (::pthread_create(tid, &attr_, pfn, arg) == 0);
  }

  bool SetPriority(ThreadPriority priority) { return true; }
  void SetMallocOpt() {
    // https://my.huhoo.net/archives/2010/05/malloptmallocnew.html
    // disable fast bins
    ::mallopt(M_MXFAST, 0);
    ::malloc_trim(0);
    ::malloc_stats();
  }
};
#endif  // defined(WIN)

Thread::Thread(const ThreadOption& option) : option_(option) {}

/**
 * use in pthread
 * pthread_t _pthreadId;
 * pthread_detach(_pthreadId);
 * */
Thread::~Thread() {
  Detach();
  started_ = false;
}

void Thread::InternalInit() {
  tid_ = CurrentThread::tid();

  CurrentThread::t_threadName = option_.name().c_str();

  LOG(INFO) << "thread name: " << CurrentThread::t_threadName;

  ThreadAttributes attr;
  attr.SetThreadName(option_.name().c_str());
  attr.SetStackSize(option_.stack_size());
  attr.BlockSigPipe();
  attr.SetMallocOpt();

  SetPriority(option_.priority());
}

void* Thread::ThreadLoop(const ThreadCallback& init_cb, CountDownLatch& latch) {
  if (init_cb) {
    init_cb();
  }
  InternalInit();
  latch.CountDown();

  started_ = true;

  try {
    option_.loop_cb();
    started_ = false;
    CurrentThread::t_threadName = "finished";
  } catch (const Exception& ex) {
    CurrentThread::t_threadName = "crashed";
    LOG(FATAL) << "exception caught in thread " << option_.name()
               << ",reason: " << ex.what()
               << "stack trace: " << ex.stackTrace();
  } catch (const std::exception& ex) {
    CurrentThread::t_threadName = "crashed";
    LOG(FATAL) << "exception caught in thread " << option_.name()
               << ",reason: " << ex.what();
  } catch (...) {
    CurrentThread::t_threadName = "crashed";
    LOG(FATAL) << "unknown exception caught in thread " << option_.name();
    throw;  // rethrow
  }
  return nullptr;
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
#elif defined(unix) || defined(__unix) || defined(__unix__)
  ::pthread_yield();
#elif defined(__APPLE__) || defined(__CYGWIN__)
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

void Thread::Start(const ThreadCallback& init_cb) {
  if (started_) {
    LOG(INFO) << "thread has started, tid: " << CurrentThread::tid();
    return;
  }
  started_ = true;
#if defined(WIN32) || defined(WIN64)
  // See bug 2902 for background on STACK_SIZE_PARAM_IS_A_RESERVATION.
  // Set the reserved stack stack size to 1M, which is the default on Windows
  // and Linux.
  th_ = ::CreateThread(nullptr, 1024 * 1024, &StartThread, this,
                       STACK_SIZE_PARAM_IS_A_RESERVATION, &thread_id_);
  if (th_ < 0) {
    LOG(FATAL) << "fail to create thread " << option_.name();
  }
#else
  try {
    CountDownLatch latch = CountDownLatch(1);
    thread_ = std::thread(
        std::bind(&Thread::ThreadLoop, this, init_cb, std::ref(latch)));
    latch.Wait();
  } catch (const Exception& ex) {
    CurrentThread::t_threadName = "crashed";
    LOG(FATAL) << "fail to create thread " << option_.name()
               << ",reason: " << ex.what()
               << "stack trace: " << ex.stackTrace();
    started_ = false;
  } catch (const std::exception& ex) {
    CurrentThread::t_threadName = "crashed";
    LOG(FATAL) << "fail to create thread " << option_.name()
               << ",reason: " << ex.what();
    started_ = false;
  } catch (...) {
    CurrentThread::t_threadName = "crashed";
    LOG(FATAL) << "fail to create thread, unkown exception " << option_.name();
    started_ = false;
    throw;  // rethrow
  }
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
 * used in pthread
 * pthread_t _pthreadId;
 * pthread_join(_pthreadId, NULL)
 * */
void Thread::Join() {
  if (!started_) {
    LOG(ERROR) << "thread not started";
    return;
  }

  if (!thread_.joinable()) {
    LOG(WARNING) << "thread not allow to join";
    return;
  }
  thread_.join();
}

void Thread::Detach() {
  if (!started_) {
    LOG(ERROR) << "thread not started";
    return;
  }

  if (!thread_.joinable()) {
    LOG(WARNING) << "thread not allow to join";
    return;
  }
  thread_.detach();
}

int Thread::Kill(int signal) {
  if (th_)
    return ::pthread_kill(thread_.native_handle(), signal);
  else
    return -EINVAL;
}

}  // namespace MSF