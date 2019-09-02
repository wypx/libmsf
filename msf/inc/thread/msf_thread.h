/**************************************************************************
*
* Copyright (c) 2017-2019, luotang.me <wypx520@gmail.com>, China.
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

#include <msf_utils.h>
#include <pthread.h>
#include <sys/syscall.h>

/*
 * 当前进程仍然处于可执行状态,但暂时"让出"处理器,
 * 使得处理器优先调度其他可执行状态的进程, 这样在
 * 进程被内核再次调度时,在for循环代码中可以期望其他进程释放锁
 * 注意,不同的内核版本对于sched_yield系统调用的实现可能是不同的,
 * 但它们的目的都是暂时"让出"处理器*/

#if (MSF_HAVE_SCHED_YIELD)
#define msf_sched_yield()           sched_yield()
#else
#define msf_sched_yield()           usleep(1)
#endif

/* pthread_self() 
 * get unique thread id in one process, but not unique in system.
 * so to get unique tis, using SYS_gettid.
 */
#define msf_gettid()                syscall(SYS_gettid)
#define msf_getpid()                getpid()
#define msf_thread_name(name)       prctl(PR_SET_NAME, name)

#define msf_spin_init(lock)         pthread_spin_init(lock, 0)
#define msf_spin_lock(lock)         pthread_spin_lock(lock)
#define msf_spin_unlock(lock)       pthread_spin_unlock(lock)

#define msf_mutex_init(mutex)       pthread_mutex_init(mutex, NULL)
#define msf_mutex_lock(mutex)       pthread_mutex_lock(mutex)
#define msf_mutex_unlock(mutex)     pthread_mutex_unlock(mutex)

#define msf_cond_init(cond)         pthread_cond_init(cond, NULL)
#define msf_cond_signal(cond)       pthread_cond_signal(cond)
#define msf_cond_wait(cond,mutex)   pthread_cond_wait(cond, mutex)

typedef void* (*pfn_msf_thrd)(void *);

s32 msf_pthread_spawn(pthread_t *tid, pfn_msf_thrd func, void *arg);

#endif
