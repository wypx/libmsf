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
#ifndef __MSF_TIME_H__
#define __MSF_TIME_H__

#include <base/Atomic.h>
#include <base/Logger.h>
using namespace MSF::BASE;

#include <sys/timeb.h>
#include <time.h>

#include <chrono>
#include <string>

namespace MSF {
namespace TIME {
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
#define time_after(a, b) ((long)((b) - (a)) < 0)
#define time_before(a, b) time_after(b, a)

#define time_after_eq(a, b) ((long)((a) - (b)) >= 0)
#define time_before_eq(a, b) time_after_eq(b, a)

/*
 * Calculate whether a is in the range of [b, c].
 */
#define time_in_range(a, b, c) (time_after_eq(a, b) && time_before_eq(a, c))

/*
 * Calculate whether a is in the range of [b, c).
 */
#define time_in_range_open(a, b, c) (time_after_eq(a, b) && time_before(a, c))

/* Same as above, but does so with platform independent 64bit types.
 * These must be used when utilizing jiffies_64 (i.e. return value of
 * get_jiffies_64() */
#define time_after64(a, b) ((int64_t)((b) - (a)) < 0)
#define time_before64(a, b) time_after64(b, a)

#define time_after_eq64(a, b) ((int64_t)((a) - (b)) >= 0)
#define time_before_eq64(a, b) time_after_eq64(b, a)

#define time_in_range64(a, b, c) \
  (time_after_eq64(a, b) && time_before_eq64(a, c))

/* see /usr/src/kernel/linux/jiffies.h */
/* extern unsigned long volatile jiffies; */
#define jiffies ((long)times(NULL))

/**
 * Return system time in nanos.
 *
 * This is a monotonicly increasing clock and
 * return the same value as System.nanoTime in java.
 */
uint64_t GetNanoTime();

#define MSF_TIMERADD(tvp, uvp, vvp) timeradd((tvp), (uvp), (vvp))
#define MSF_TIMERSUB(tvp, uvp, vvp) timersub((tvp), (uvp), (vvp))
#define MSF_TIMERCLEAR(tvp) timerclear(tvp)
#define MSF_TIMERISSET(tvp) timerisset(tvp)

/** Return true iff the tvp is related to uvp according to the relational
 * operator cmp.  Recognized values for cmp are ==, <=, <, >=, and >. */
#define MSF_TIMERCMP(tvp, uvp, cmp)                                      \
  (((tvp)->tv_sec == (uvp)->tv_sec) ? ((tvp)->tv_usec cmp(uvp)->tv_usec) \
                                    : ((tvp)->tv_sec cmp(uvp)->tv_sec))

void SetTimespecRelative(struct timespec *p_ts, long long msec);
void SleepMsec(long long msec);

uint64_t CurrentMilliTime();

typedef std::chrono::high_resolution_clock::time_point TimePoint;
TimePoint CurrentMilliTimePoint();
double GetDurationMilliSecond(const TimePoint t1, const TimePoint t2);

typedef std::function<void(void)> ExecutorFunc;
void GetExecuteTime(ExecutorFunc func);

void msf_time_init(void);

}  // namespace TIME
}  // namespace MSF
#endif
