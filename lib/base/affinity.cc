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
#include <base/logging.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#include <algorithm>

#include "os.h"

using namespace MSF;

namespace MSF {

struct getcpu_cache {
  unsigned long blob[128 / sizeof(long)];
};

typedef long (*vgetcpu_fn)(unsigned *cpu, unsigned *node,
                           struct getcpu_cache *tcache);
vgetcpu_fn vgetcpu;

/*查看在哪个核心上切换*/
int msf_init_vgetcpu(void) {
  // void *vdso;

  // MSF_DLERROR();
  // vdso = MSF_DLOPEN_L("linux-vdso.so.1");
  // if (!vdso)
  //     return -1;
  // vgetcpu = (vgetcpu_fn)MSF_DLSYM(vdso, "__vdso_getcpu");
  // MSF_DLCLOSE(vdso);
  // return !vgetcpu ? -1 : 0;
  return 0;
}

uint32_t msf_get_current_cpu(void) {
  bool first = true;
  uint32_t cpu;

  if (!first && vgetcpu) {
    vgetcpu(&cpu, NULL, NULL);
    return cpu;
  }

  if (!first) {
    return sched_getcpu();
  }

  first = 0;
  if (msf_init_vgetcpu() < 0) {
    vgetcpu = NULL;
    return sched_getcpu();
  }
  vgetcpu(&cpu, NULL, NULL);
  return cpu;
}

int thread_pin_to_cpu(uint32_t cpu_id) {
  int rc;

  cpu_set_t cpu_info;
  CPU_ZERO(&cpu_info);
  CPU_SET(cpu_id, &cpu_info);
  rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_info);

  if (rc < 0) {
    // LOG(ERROR) << "set thread(%u) affinity failed.", cpu_id);
  }

#if 0
    for (s32 j = 0; j < srv->max_cores; j++) {
        if (CPU_ISSET(j, &cpu_info)) {
            printf("CPU %d\n", j);
        }
    }
#endif
  return rc;
}

#if (MSF_HAVE_CPUSET_SETAFFINITY)

#include <sys/cpuset.h>

int process_pin_to_cpu(uint32_t cpu_id) {
  s32 rc = -1;

  cpu_set_t mask;
  cpu_set_t get;

  CPU_ZERO(&mask);
  CPU_SET(cpu_id, &mask);

  if (numa_num_task_cpus() > CPU_SETSIZE) return -1;

  if (CPU_COUNT(&mask) == 1) return 0;

  if (cpuset_setaffinity(CPU_LEVEL_WHICH, CPU_WHICH_PID, -1, sizeof(cpuset_t),
                         &mask) < 0) {
    LOG(ERROR) << "Could not set CPU affinity, continuing.";
  }

  /* guaranteed to take effect immediately */
  sched_yield();

  return rc;
}

#elif(MSF_HAVE_SCHED_SETAFFINITY)

int process_pin_to_cpu(uint32_t cpu_id) {
  s32 rc = -1;

  cpu_set_t mask;
  cpu_set_t get;

  CPU_ZERO(&mask);
  CPU_SET(cpu_id, &mask);

  if (numa_num_task_cpus() > CPU_SETSIZE) return -1;

  if (CPU_COUNT(&mask) == 1) return 0;

  if (::sched_setaffinity(0, sizeof(mask), &mask) < 0) {
    LOG(ERROR) << "Could not set CPU affinity, continuing.";
  }

  /* guaranteed to take effect immediately */
  ::sched_yield();

  CPU_ZERO(&get);
  if (::sched_getaffinity(0, sizeof(get), &get) == -1) {
    LOG(ERROR) << "Could not get CPU affinity, continuing.";
  }

  for (s32 i = 0; i < get_nprocs_conf(); i++) {
    if (CPU_ISSET(i, &get)) {
      // LOG(ERROR) << "this process %d is running processor: %d.", i, i);
    }
  }

  return rc;
}

/***/
void set_cpu_affinity(uint16_t cpu_id) {
#if defined(__CYGWIN__)
// setting cpu affinity on cygwin is not supported
#elif defined(_WIN32)
  // core number starts from 0
  auto mask = (static_cast<DWORD_PTR>(1) << cpu_id);
  auto ret = SetThreadAffinityMask(GetCurrentThread(), mask);
  if (ret == 0) {
    auto const last_error =
        std::error_code(GetLastError(), std::system_category());

    std::ostringstream error_msg;
    error_msg << "failed to call set_cpu_affinity, with error message "
              << "\"" << last_error.message() << "\", errno \""
              << last_error.value() << "\"";
    QUILL_THROW(QuillError{error_msg.str()});
  }
#elif defined(__APPLE__)
  // I don't think that's possible to link a thread with a specific core with
  // Mac OS X
  // This may be used to express affinity relationships  between threads in the
  // task.
  // Threads with the same affinity tag will be scheduled to share an L2 cache
  // if possible.
  thread_affinity_policy_data_t policy = {cpu_id};

  // Get the mach thread bound to this thread
  thread_port_t mach_thread = pthread_mach_thread_np(pthread_self());

  thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY,
                    (thread_policy_t) & policy, 1);
#else
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu_id, &cpuset);

  auto const err = ::sched_setaffinity(0, sizeof(cpuset), &cpuset);

  if (unlikely(err == -1)) {
    std::ostringstream error_msg;
    error_msg << "failed to call set_cpu_affinity, with error message "
              << "\"" << strerror(errno) << "\", errno \"" << errno << "\"";
  }
#endif
}

/***/
void set_thread_name(char const *name) {
#if defined(__CYGWIN__)
// set thread name on cygwin not supported
#elif defined(__MINGW32__) || defined(__MINGW64__)
  // Disabled on MINGW.
  (void)name;
#elif defined(_WIN32)
  std::wstring name_ws = s2ws(name);
  // Set the thread name
  HRESULT hr = SetThreadDescription(GetCurrentThread(), name_ws.data());
  if (FAILED(hr)) {
    QUILL_THROW(QuillError{"Failed to set thread name"});
  }
#elif defined(__APPLE__)
  auto const res = pthread_setname_np(name);
  if (res != 0) {
    QUILL_THROW(
        QuillError{"Failed to set thread name. error: " + std::to_string(res)});
  }
#else
  auto const err =
      prctl(PR_SET_NAME, reinterpret_cast<unsigned long>(name), 0, 0, 0);

  if (QUILL_UNLIKELY(err == -1)) {
    std::ostringstream error_msg;
    error_msg << "failed to call set_thread_name, with error message "
              << "\"" << strerror(errno) << "\", errno \"" << errno << "\"";
    QUILL_THROW(QuillError{error_msg.str()});
  }
#endif
}

/***/
uint32_t get_thread_id() noexcept {
#if defined(__CYGWIN__)
  // get thread id on cygwin not supported
  return 0;
#elif defined(_WIN32)
  return static_cast<uint32_t>(GetCurrentThreadId());
#elif defined(__linux__)
  return static_cast<uint32_t>(::syscall(SYS_gettid));
#elif defined(__APPLE__)
  uint64_t tid64;
  pthread_threadid_np(nullptr, &tid64);
  return static_cast<uint32_t>(tid64);
#endif
}

/***/
uint32_t get_process_id() noexcept {
#if defined(__CYGWIN__)
  // get pid on cygwin not supported
  return 0;
#elif defined(_WIN32)
  return static_cast<uint32_t>(GetCurrentProcessId());
#else
  return static_cast<uint32_t>(getpid());
#endif
}

#else

/*
 * If any process is being supervised/subscribed, and watchdogd is
 * enabled, we raise the RT priority to 98 (just below the kernel WDT in
 * prio).  This to ensure that system monitoring goes before anything
 * else in the system.
 */
void set_priority(int enabled = 0, int rtprio = 0) {
  int result = 0;
  struct sched_param prio;

  if (enabled) {
    // DEBUG("Setting SCHED_RR rtprio %d", rtprio);
    prio.sched_priority = rtprio;
    result = ::sched_setscheduler(getpid(), SCHED_RR, &prio);
  } else {
    // DEBUG("Setting SCHED_OTHER prio %d", 0);
    prio.sched_priority = 0;
    result = ::sched_setscheduler(getpid(), SCHED_OTHER, &prio);
  }

  if (result) {
    LOG(ERROR) << "Failed to set scheduler priority enabled: " << enabled;
  }
}

int process_pin_to_cpu(uint32_t cpu_id) { return 0; }
#endif

/*
 *nice:
 * int nice(int increment)
 * {
 *      int old = getpriority(PRIO_PROCESS, msf_getpid());
 *      return setpriority(PRIO_PROCESS, msf_getpid(), increment+old);
 * }
 * increment 数字越大则优先越排在后面,进程执行得越慢
 * increment为负,只有超级用户才能设置,表明进程的优先级越高
 */
int msf_set_nice(int increment) { return nice(increment); }

/*priority:0-10, max is 10, default is 5*/
int msf_set_priority(int which, int who, int priority) {
  int rc;

  rc = setpriority(which, who, priority);
  if (rc == -1) {
    LOG(ERROR) << "Set priority failed";
    return -1;
  }

  return 0;
}

/*当指定的一组进程优先级不同时,返回优先级最小的
 *如果返回值是-1, 有两种情况:
 *-1可能是进程的优先级,这个时候errno=0,如果不为0,则是出错了*/
int msf_get_priority(int which, int who) {
  return getpriority(PRIO_PROCESS, getpid());
}

#define MSF_SET_SPEC_PROC_PRIORITY(priority) \
  msf_set_priority(PRIO_PROCESS, getpid(), priority)
#define MSF_SET_GROUP_PROC_PRIORITY(priority) \
  msf_set_priority(PRIO_PGRP, getpid(), priority)
#define MSF_SET_USER_PROC_PRIORITY(priority) \
  msf_set_priority(PRIO_USER, getpid(), priority)

}  // namespace MSF