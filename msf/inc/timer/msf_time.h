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
#ifndef __MSF_TIME_H__
#define __MSF_TIME_H__

#include <time.h>
#include <sys/time.h>
#include <msf_utils.h>

/*
#include <linux/types.h>
#include <time.h>
#include <sys/timex.h>
*/

/*
 *  These inlines deal with timer wrapping correctly. You are
 *  strongly encouraged to use them
 *  1. Because people otherwise forget
 *  2. Because if the timer wrap changes in future you won't have to
 *     alter your driver code.
 *
 * time_after(a,b) returns true if the time a is after time b.
 *
 * Do this with "<0" and ">=0" to only test the sign of the result. A
 * good compiler would generate better code (and a really good compiler
 * wouldn't care). Gcc is currently neither.
 */
#define time_after(a,b)     \
     ((long)((b) - (a)) < 0)
#define time_before(a,b)    time_after(b,a)

#define time_after_eq(a,b)  \
     ((long)((a) - (b)) >= 0)
#define time_before_eq(a,b) time_after_eq(b,a)

/*
 * Calculate whether a is in the range of [b, c].
 */
#define time_in_range(a,b,c) \
    (time_after_eq(a,b) && \
     time_before_eq(a,c))

/*
 * Calculate whether a is in the range of [b, c).
 */
#define time_in_range_open(a,b,c) \
    (time_after_eq(a,b) && \
     time_before(a,c))

/* Same as above, but does so with platform independent 64bit types.
 * These must be used when utilizing jiffies_64 (i.e. return value of
 * get_jiffies_64() */
#define time_after64(a,b)   \
     ((s64)((b) - (a)) < 0)
#define time_before64(a,b)  time_after64(b,a)

#define time_after_eq64(a,b)    \
     ((s64)((a) - (b)) >= 0)
#define time_before_eq64(a,b)	time_after_eq64(b,a)

#define time_in_range64(a, b, c) \
    (time_after_eq64(a, b) && \
     time_before_eq64(a, c))

#define msf_tm_sec            tm_sec
#define msf_tm_min            tm_min
#define msf_tm_hour           tm_hour
#define msf_tm_mday           tm_mday
#define msf_tm_mon            tm_mon
#define msf_tm_year           tm_year
#define msf_tm_wday           tm_wday
#define msf_tm_isdst          tm_isdst

void msf_timezone_update(void);
void msf_localtime(time_t s, struct tm *t);
void msf_libc_localtime(time_t s, struct tm *t);
void msf_libc_gmtime(time_t s, struct tm *t);

#define MSF_TIMEZONE(isdst)   (- (isdst ? timezone + 3600 : timezone) / 60)
#define MSF_GETIMEOFDAY(tp)  (void) gettimeofday(tp, NULL);
#define MSF_MLEEP(ms)        (void) usleep(ms * 1000)
#define MSF_SLEEP(s)          (void) sleep(s)

#ifdef MSF_HAVE_LINUX_TIMER
#define MSF_TIMERADD(tvp, uvp, vvp) timeradd((tvp), (uvp), (vvp))
#define MSF_TIMERSUB(tvp, uvp, vvp) timersub((tvp), (uvp), (vvp))
#define MSF_TIMERCLEAR(tvp) timerclear(tvp)
#define MSF_TIMERISSET(tvp) timerisset(tvp)
#else
#define MSF_TIMERADD(tvp, uvp, vvp)     \
    do {                                \
        (vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;      \
        (vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;   \
        if ((vvp)->tv_usec >= 1000000) {        \
            (vvp)->tv_sec++;                    \
            (vvp)->tv_usec -= 1000000;          \
        }                                       \
    } while (0)

#define MSF_TIMERSUB(tvp, uvp, vvp)                         \
    do {                                                    \
        (vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;      \
        (vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;   \
        if ((vvp)->tv_usec < 0) {                           \
            (vvp)->tv_sec--;                                \
            (vvp)->tv_usec += 1000000;                      \
        }                                                   \
    } while (0)

#define MSF_TIMERCLEAR(tvp) (tvp)->tv_sec = (tvp)->tv_usec = 0
#define MSF_TIMERISSET(tvp) ((tvp)->tv_sec || (tvp)->tv_usec)
#endif

/** Return true iff the tvp is related to uvp according to the relational
 * operator cmp.  Recognized values for cmp are ==, <=, <, >=, and >. */
#define MSF_TIMERCMP(tvp, uvp, cmp)         \
    (((tvp)->tv_sec == (uvp)->tv_sec) ?     \
     ((tvp)->tv_usec cmp (uvp)->tv_usec) :  \
     ((tvp)->tv_sec cmp (uvp)->tv_sec))

     /* see /usr/src/kernel/linux/jiffies.h */
     /* extern unsigned long volatile jiffies; */
     
#define jiffies     ((long)times(NULL))

static inline int msf_clock_gettime(struct timespec *ts) {
    return clock_gettime(CLOCK_MONOTONIC, ts);
}


#endif
