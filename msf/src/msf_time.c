/**************************************************************************
*
* Copyright (c) 2017-2018, luotang.me <wypx520@gmail.com>, China.
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
#include <msf_atomic.h>
#include <msf_time.h>


static s8  *week[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static s8  *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

static msf_atomic_t  msf_time_lock;
volatile u32   msf_current_msec; //格林威治时间1970年1月1日凌晨0点0分0秒到当前时间的毫秒数


void msf_time_update(void) {

    time_t           sec;
    u32              msec;
    struct timeval	 tv;

    if (!msf_trylock(&msf_time_lock)) {
        return;
    }

    MSF_GETIMEOFDAY(&tv);
    sec = tv.tv_sec;
    msec = tv.tv_usec / 1000;

    msf_current_msec = (u32) sec * 1000 + msec;


    /* 禁止编译器对后面的语句优化,如果没有这个限制,
     * 编译器可能将前后两部分代码合并,可能导致这6个时间更新出现间隔
     * 期间若被读取会出现时间不一致的情况*/
    msf_memory_barrier();


    msf_unlock(&msf_time_lock);
}

static u32 msf_monotonic_time(time_t sec, u32 msec) {
#if (NGX_HAVE_CLOCK_MONOTONIC)
    struct timespec  ts;

#if defined(CLOCK_MONOTONIC_FAST)
    clock_gettime(CLOCK_MONOTONIC_FAST, &ts);

#elif defined(CLOCK_MONOTONIC_COARSE)
    clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);

#else
    clock_gettime(CLOCK_MONOTONIC, &ts);
#endif

    sec = ts.tv_sec;
    msec = ts.tv_nsec / 1000000;

#endif

    return (u32) sec * 1000 + msec;
}


void msf_time_init(void) {

    struct timeval  tv;
    time_t          sec;
    u32             msec;
    struct tm       tm, gmt;

    MSF_GETIMEOFDAY(&tv);

    sec = tv.tv_sec;
    msec = tv.tv_usec / 1000;

    msf_current_msec = msf_monotonic_time(sec, msec);

    msf_time_update();
}
