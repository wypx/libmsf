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
#include "Thread.h"

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

#include "Exception.h"

using namespace MSF;
using namespace MSF::THREAD;

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
  return uint64_t(pthread_mach_thread_np(pthread_self()));
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
#if __APPLE__
  uint64_t tid;
  pthread_threadid_np(nullptr, &tid);
  return tid;
#elif defined(_WIN32)
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

namespace THREAD {

int pthreadSpawn(pthread_t* tid, void* (*pfn)(void*), void* arg) {
  int rc;
  pthread_attr_t thread_attr;

#ifndef WIN32
  sigset_t signal_mask;
  sigemptyset(&signal_mask);
  sigaddset(&signal_mask, SIGPIPE);
  rc = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);
  if (rc != 0) {
    LOG(ERROR) << "Block sigpipe error.";
  }
#endif

  pthread_attr_init(&thread_attr);
  pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
  if (pthread_create(tid, &thread_attr, pfn, arg) < 0) {
    LOG(ERROR) << "Do pthread_create errno: " << errno;
    return -1;
  }

  return 0;
}

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

Thread::Thread(ThreadFunc func, const std::string& name)
    : _started(false), _tid(0), _func(std::move(func)), _name(name), _latch(1) {
  setDefaultName();
}

/**
 * use in pthread
 * pthread_t _pthreadId;
 * pthread_detach(_pthreadId);
 * */
Thread::~Thread() {
  if (_started && !_thread.joinable()) {
    _thread.detach();
  }
  _started = false;
}

void* Thread::threadLoop(ThreadFunc initFunc) {
  if (initFunc) {
    initFunc();
  }
  _tid = CurrentThread::tid();
  CurrentThread::t_threadName = _name.c_str();
  LOG(INFO) << "Thread name: " << CurrentThread::t_threadName
            << " name:" << _name;
  ::prctl(PR_SET_NAME, CurrentThread::t_threadName);

  _latch.countDown();

  try {
    _func();
    _started = false;
    CurrentThread::t_threadName = "finished";
  }
  catch (const Exception& ex) {
    CurrentThread::t_threadName = "crashed";
    LOG(FATAL) << "exception caught in ThreadPool " << _name.c_str()
               << ",reason: " << ex.what()
               << "stack trace: " << ex.stackTrace();
    abort();
  }
  catch (const std::exception& ex) {
    CurrentThread::t_threadName = "crashed";
    LOG(FATAL) << "exception caught in ThreadPool " << _name.c_str()
               << ",reason: " << ex.what();
    abort();
  }
  catch (...) {
    CurrentThread::t_threadName = "crashed";
    LOG(FATAL) << "unknown exception caught in ThreadPool " << _name.c_str();
    throw;  // rethrow
  }
  return nullptr;
}

void Thread::setDefaultName() {
  // int num = _numCreated.fetch_add(1);
  if (_name.empty()) {
    _name = "Thread_" + std::to_string(gettid());
  }
}

/**
 * pthread_t _pthreadId;
 * pthreadSpawn(&_pthreadId, startThread, data)
 * */
void Thread::start(const ThreadFunc& initFunc) {
  assert(!_started);
  _started = true;

  try {
    _thread = std::thread(std::bind(&Thread::threadLoop, this, initFunc));
    _latch.wait();
  }
  catch (const Exception& ex) {
    CurrentThread::t_threadName = "crashed";
    LOG(FATAL) << "Fail to create thread ThreadPool " << _name.c_str()
               << ",reason: " << ex.what()
               << "stack trace: " << ex.stackTrace();
    _started = false;
  }
  catch (const std::exception& ex) {
    CurrentThread::t_threadName = "crashed";
    LOG(FATAL) << "Fail to create thread ThreadPool " << _name.c_str()
               << ",reason: " << ex.what();
    _started = false;
  }
  catch (...) {
    CurrentThread::t_threadName = "crashed";
    LOG(FATAL) << "Fail to create thread ThreadPool, unkown exception "
               << _name.c_str();
    _started = false;
    throw;  // rethrow
  }
}

/**
 * used in pthread
 * pthread_t _pthreadId;
 * pthread_join(_pthreadId, NULL)
 * */
void Thread::join() {
  if (!_started) {
    LOG(ERROR) << "Thread already started";
    return;
  }

  if (!_thread.joinable()) {
    LOG(WARNING) << "Thread not allow to join";
    return;
  }
  _thread.join();
}

}  // namespace THREAD
}  // namespace MSF
