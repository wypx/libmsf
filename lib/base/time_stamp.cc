#include <stdio.h>
#include <sys/time.h>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>

#include "time_stamp.h"

using namespace MSF;

static_assert(sizeof(TimeStamp) == sizeof(int64_t),
              "TimeStamp is same size as int64_t");

std::string TimeStamp::toString() const {
  char buf[32] = {0};
  int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
  int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
  snprintf(buf, sizeof(buf), "%" PRId64 ".%06" PRId64 "", seconds,
           microseconds);
  return buf;
}

std::string TimeStamp::toFormattedString(bool showMicroseconds) const {
  char buf[64] = {0};
  time_t seconds =
      static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
  struct tm tm_time;
  gmtime_r(&seconds, &tm_time);

  if (showMicroseconds) {
    int microseconds =
        static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, microseconds);
  } else {
    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
  }
  return buf;
}

TimeStamp TimeStamp::now() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  int64_t seconds = tv.tv_sec;
  return TimeStamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}
