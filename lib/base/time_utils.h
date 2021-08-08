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
#ifndef BASE_TIME_H_
#define BASE_TIME_H_

#include <sys/time.h>
#include <sys/timeb.h>
#include <time.h>

#include <chrono>
#include <ctime>
#include <functional>
#include <string>

#include "atomic.h"

using namespace MSF;

namespace MSF {
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

void SetTimespecRelative(struct timespec* p_ts, long long msec);
void SleepMsec(long long msec);

uint64_t CurrentMilliTime();

typedef std::chrono::high_resolution_clock::time_point TimePoint;

TimePoint CurrentMilliTimePoint();
double GetDurationMilliSecond(const TimePoint t1, const TimePoint t2);

typedef std::function<void(void)> ExecutorFunc;
void GetExecuteTime(ExecutorFunc func);

void msf_time_init(void);

static const int64_t kNumMillisecsPerSec = INT64_C(1000);
static const int64_t kNumMicrosecsPerSec = INT64_C(1000000);
static const int64_t kNumNanosecsPerSec = INT64_C(1000000000);

static const int64_t kNumMicrosecsPerMillisec =
    kNumMicrosecsPerSec / kNumMillisecsPerSec;
static const int64_t kNumNanosecsPerMillisec =
    kNumNanosecsPerSec / kNumMillisecsPerSec;
static const int64_t kNumNanosecsPerMicrosec =
    kNumNanosecsPerSec / kNumMicrosecsPerSec;

// TODO(honghaiz): Define a type for the time value specifically.

class ClockInterface {
 public:
  virtual ~ClockInterface() {}
  virtual int64_t TimeNanos() const = 0;
};

// Sets the global source of time. This is useful mainly for unit tests.
//
// Returns the previously set ClockInterface, or nullptr if none is set.
//
// Does not transfer ownership of the clock. SetClockForTesting(nullptr)
// should be called before the ClockInterface is deleted.
//
// This method is not thread-safe; it should only be used when no other thread
// is running (for example, at the start/end of a unit test, or start/end of
// main()).
//
// TODO(deadbeef): Instead of having functions that access this global
// ClockInterface, we may want to pass the ClockInterface into everything
// that uses it, eliminating the need for a global variable and this function.
ClockInterface* SetClockForTesting(ClockInterface* clock);

// Returns previously set clock, or nullptr if no custom clock is being used.
ClockInterface* GetClockForTesting();

#if defined(WINUWP)
// Synchronizes the current clock based upon an NTP server's epoch in
// milliseconds.
void SyncWithNtp(int64_t time_from_ntp_server_ms);
#endif  // defined(WINUWP)

// Returns the actual system time, even if a clock is set for testing.
// Useful for timeouts while using a test clock, or for logging.
int64_t SystemTimeNanos();
int64_t SystemTimeMillis();

// Returns the current time in milliseconds in 32 bits.
uint32_t Time32();

// Returns the current time in milliseconds in 64 bits.
int64_t TimeMillis();
// Deprecated. Do not use this in any new code.
inline int64_t Time() { return TimeMillis(); }

// Returns the current time in microseconds.
int64_t TimeMicros();

// Returns the current time in nanoseconds.
int64_t TimeNanos();

// Returns a future timestamp, 'elapsed' milliseconds from now.
int64_t TimeAfter(int64_t elapsed);

// Number of milliseconds that would elapse between 'earlier' and 'later'
// timestamps.  The value is negative if 'later' occurs before 'earlier'.
int64_t TimeDiff(int64_t later, int64_t earlier);
int32_t TimeDiff32(uint32_t later, uint32_t earlier);

// The number of milliseconds that have elapsed since 'earlier'.
inline int64_t TimeSince(int64_t earlier) { return TimeMillis() - earlier; }

// The number of milliseconds that will elapse between now and 'later'.
inline int64_t TimeUntil(int64_t later) { return later - TimeMillis(); }

class TimestampWrapAroundHandler {
 public:
  TimestampWrapAroundHandler();

  int64_t Unwrap(uint32_t ts);

 private:
  uint32_t last_ts_;
  int64_t num_wrap_;
};

// Convert from tm, which is relative to 1900-01-01 00:00 to number of
// seconds from 1970-01-01 00:00 ("epoch"). Don't return time_t since that
// is still 32 bits on many systems.
int64_t TmToSeconds(const tm& tm);

// Return the number of microseconds since January 1, 1970, UTC.
// Useful mainly when producing logs to be correlated with other
// devices, and when the devices in question all have properly
// synchronized clocks.
//
// Note that this function obeys the system's idea about what the time
// is. It is not guaranteed to be monotonic; it will jump in case the
// system time is changed, e.g., by some other process calling
// settimeofday. Always use rtc::TimeMicros(), not this function, for
// measuring time intervals and timeouts.
int64_t TimeUTCMicros();

// Return the number of milliseconds since January 1, 1970, UTC.
// See above.
int64_t TimeUTCMillis();

}  // namespace MSF
#endif
