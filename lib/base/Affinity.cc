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
#include <butil/logging.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#include <algorithm>

#include "Os.h"

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

  if (sched_setaffinity(0, sizeof(mask), &mask) < 0) {
    LOG(ERROR) << "Could not set CPU affinity, continuing.";
  }

  /* guaranteed to take effect immediately */
  sched_yield();

  CPU_ZERO(&get);
  if (sched_getaffinity(0, sizeof(get), &get) == -1) {
    LOG(ERROR) << "Could not get CPU affinity, continuing.";
  }

  for (s32 i = 0; i < get_nprocs_conf(); i++) {
    if (CPU_ISSET(i, &get)) {
      // LOG(ERROR) << "this process %d is running processor: %d.", i, i);
    }
  }

  return rc;
}
#else

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
