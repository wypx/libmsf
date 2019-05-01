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
#include <msf_utils.h>
#include <pthread.h>
#include <sys/syscall.h>

/*
 * 当前进程仍然处于可执行状态,但暂时"让出"处理器,
 * 使得处理器优先调度其他可执行状态的进程,这样,
 * 在进程被内核再次调度时,在for循环代码中可以期望其他进程释放锁
 * 注意,不同的内核版本对于ngx_sched_yield系统调用的实现可能是不同的,
 * 但它们的目的都是暂时"让出"处理器*/

#if (MSF_HAVE_SCHED_YIELD)
#define msf_sched_yield()  sched_yield()
#else
#define msf_sched_yield()  usleep(1)
#endif

#define msf_getpid() getpid()

/* pthread_self() 
 * get unique thread id in one process, but not unique in system.
 * so to get unique tis, using SYS_gettid.
 */
#define msf_gettid() syscall(SYS_gettid)

#define msf_thread_name(name)  prctl(PR_SET_NAME, name) 

s32 pthread_spawn(pthread_t *tid, void *(*func)(void *), void *arg);
