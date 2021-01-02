#include <stdint.h>

#if defined(__linux__)
#include <sys/time.h>
#if defined(__apple__)
#include <mach/mach_time.h>
#endif
#endif

#if defined(WEBRTC_WIN)
// clang-format off
// clang formatting would put <windows.h> last,
// which leads to compilation failure.
#include <windows.h>
#include <mmsystem.h>
#include <sys/timeb.h>
// clang-format on
#endif

#include "safe_conversions.h"
#include "time_utils.h"

using namespace MSF;

namespace MSF {

static AtomicOps::atomic_t kTimeLock;

// 格林威治时间1970年1月1日凌晨0点0分0秒到当前时间的毫秒数
static volatile uint32_t g_current_msec;

static void msf_time_update(void) {
  time_t sec;
  uint32_t msec;
  struct timeval tv;

  if (!AtomicTryLock(&kTimeLock)) {
    return;
  }

  gettimeofday(&tv, NULL);
  sec = tv.tv_sec;
  msec = tv.tv_usec / 1000;

  g_current_msec = (uint32_t)sec * 1000 + msec;

  /* 禁止编译器对后面的语句优化,如果没有这个限制,
   * 编译器可能将前后两部分代码合并,可能导致这6个时间更新出现间隔
   * 期间若被读取会出现时间不一致的情况*/
  MemoryBarrier();

  AtomicUnLock(&kTimeLock);

  // LOG(INFO) << "Current time msec: " << g_current_msec;
}

static uint32_t msf_monotonic_time(time_t sec, uint32_t msec) {
#if (MSF_HAVE_CLOCK_MONOTONIC)
  struct timespec ts;

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

  return (uint32_t)sec * 1000 + msec;
}

static inline int msf_clock_gettime(struct timespec* ts) {
  return ::clock_gettime(CLOCK_MONOTONIC, ts);
}

#define NS_PER_S 1000000000
void SetTimespecRelative(struct timespec* p_ts, long long msec) {
  struct timeval tv;

  ::gettimeofday(&tv, (struct timezone*)nullptr);

  p_ts->tv_sec = tv.tv_sec + (msec / 1000);
  p_ts->tv_nsec = (tv.tv_usec + (msec % 1000) * 1000L) * 1000L;
  /* assuming tv.tv_usec < 10^6 */
  if (p_ts->tv_nsec >= NS_PER_S) {
    p_ts->tv_sec++;
    p_ts->tv_nsec -= NS_PER_S;
  }
}

void SleepMsec(long long msec) {
  struct timespec ts;
  int err;

  ts.tv_sec = (msec / 1000);
  ts.tv_nsec = (msec % 1000) * 1000 * 1000;

  do {
    err = ::nanosleep(&ts, &ts);
  } while (err < 0 && errno == EINTR);
}

uint64_t GetNanoTime() {
  struct timespec now;
  ::clock_gettime(CLOCK_MONOTONIC, &now);
  return now.tv_sec * 1000000000LL + now.tv_nsec;
}

uint64_t CurrentMilliTime() {
#if _MSC_VER
  extern "C" std::uint64_t __rdtsc();
#pragma intrinsic(__rdtsc)
  return __rdtsc();
#elif __GNUC__&&(__i386__ || __x86_64__)
  return __builtin_ia32_rdtsc();
#else
  /* use steady_clock::now() as an approximation for the timestamp counter on
   * non-x86 systems. */
  return std::chrono::steady_clock::now().time_since_epoch().count();

  // std::chrono::steady_clock::time_point t1;
  auto tp = std::chrono::time_point_cast<std::chrono::milliseconds>(
      std::chrono::high_resolution_clock::now());
  // std::chrono::steady_clock::duration dt;
  auto tmp = std::chrono::duration_cast<std::chrono::milliseconds>(
      tp.time_since_epoch());
  return tmp.count();
#endif
}

std::time_t now() {
#if 0  // no need for chrono here yet
    std::chrono::time_point<std::chrono::system_clock> system_now = std::chrono::system_clock::now();
    return std::chrono::system_clock::to_time_t(system_now);
#endif
  return std::time(0);
}

TimePoint CurrentMilliTimePoint() {
  return std::chrono::high_resolution_clock::now();
}

double GetDurationMilliSecond(const TimePoint t1, const TimePoint t2) {
  std::chrono::duration<double, std::ratio<1, 1000>> durationMs =
      std::chrono::duration_cast<
          std::chrono::duration<double, std::ratio<1, 1000>>>(t2 - t1);
  return durationMs.count();
}

static inline uint64_t getCurrentMicrosecondOrigin() {
#if !defined(_WIN32)
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000LL + tv.tv_usec;
#else
  return std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
#endif
}

// ns
// https://www.cnblogs.com/lzpong/p/6544117.html
// https://blog.csdn.net/Y__Jason/article/details/78038991
// https://www.jianshu.com/p/c9b775d831fb
// https://blog.csdn.net/cw_hello1/article/details/66476290
// https://www.cnblogs.com/jwk000/p/3560086.html
// https://www.cnblogs.com/zhongpan/p/7490657.html
/**
 * chrono库主要包含了三种类型: 时间间隔duration、时钟clock和时间点time_point
 * https://blog.csdn.net/u013390476/article/details/50209603
 * https://blog.csdn.net/oncealong/article/details/28599655
 * https://www.cnblogs.com/zhongpan/p/7490657.html
 * https://www.jianshu.com/p/c9b775d831fb
 * */
void GetExecuteTime(ExecutorFunc func) {
  // auto tp =
  //     std::chrono::time_point_cast<std::chrono::milliseconds>(
  //         std::chrono::high_resolution_clock::now());
  // auto tmp =
  //     std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
  // return tmp.count();

  auto start = std::chrono::high_resolution_clock::now();
  func();
  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  // LOG(INFO)
  //     << "Cost "
  //     << std::chrono::duration_cast<std::chrono::hours>(end - start).count()
  //     << " hours";

  // LOG(INFO)
  //     << "Cost "
  //     << std::chrono::duration_cast<std::chrono::minutes>(end -
  //     start).count()
  //     << " minutes";
  std::chrono::duration<double, std::ratio<60, 1>> duration_min =
      std::chrono::duration_cast<
          std::chrono::duration<double, std::ratio<60, 1>>>(end - start);
  // LOG(INFO) << "cost " << duration_min.count() << " minutes";

  std::chrono::duration<double, std::ratio<1, 1>> duration_s(end - start);
  // LOG(INFO) << "cost " << duration_s.count() << " seconds";
  // LOG(INFO)
  //     << "cost "
  //     << std::chrono::duration_cast<std::chrono::seconds>(end -
  //     start).count()
  //     << " seconds";

  // https://blog.csdn.net/Y__Jason/article/details/78038991
  // 会被截断可能
  std::chrono::duration<double> time_span =
      std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
  // LOG(INFO) << "It took me " << time_span.count() << " seconds.";

  // LOG(INFO) << "Cost time: "
  //           << double(duration.count()) *
  //                  std::chrono::microseconds::period::num /
  //                  std::chrono::microseconds::period::den
  //           << " seconds";

  std::chrono::duration<double, std::ratio<1, 1000>> duration_ms =
      std::chrono::duration_cast<
          std::chrono::duration<double, std::ratio<1, 1000>>>(end - start);
  // LOG(INFO) << "cost " << duration_ms.count() << " milliseconds";
  // LOG(INFO) << "cost "
  //           << std::chrono::duration_cast<std::chrono::milliseconds>(end -
  //                                                                    start)
  //                  .count()
  //           << " milliseconds";

  std::chrono::duration<double, std::ratio<1, 1000000>> duration_mcs =
      std::chrono::duration_cast<
          std::chrono::duration<double, std::ratio<1, 1000000>>>(end - start);
  // LOG(INFO) << "cost " << duration_mcs.count() << " microseconds";
  // LOG(INFO) << "cost "
  //           << std::chrono::duration_cast<std::chrono::microseconds>(end -
  //                                                                    start)
  //                  .count()
  //           << " microseconds";

  std::chrono::duration<double, std::ratio<1, 1000000000>> duration_ns =
      std::chrono::duration_cast<
          std::chrono::duration<double, std::ratio<1, 1000000000>>>(end -
                                                                    start);
  // LOG(INFO) << "cost " << duration_ns.count() << " nanoseconds";
  // LOG(INFO) << "cost "
  //           << std::chrono::duration_cast<std::chrono::nanoseconds>(end -
  //           start)
  //                  .count()
  //           << " nanoseconds";
}

void msf_time_init(void) {
  struct timeval tv;
  time_t sec;
  uint32_t msec;
  // struct tm       tm, gmt;

  gettimeofday(&tv, NULL);

  sec = tv.tv_sec;
  msec = tv.tv_usec / 1000;

  g_current_msec = msf_monotonic_time(sec, msec);

  msf_time_update();
}

std::tm toLocal(const std::time_t& time) {
  std::tm tm_snapshot;
#if defined(WIN32) || defined(WIN64)
  ::localtime_s(&tm_snapshot, &time);  // thread-safe?
#else
  ::localtime_r(&time, &tm_snapshot);  // POSIX
#endif
  return tm_snapshot;
}

std::tm toUTC(const std::time_t& time) {
  // TODO: double check thread safety of native methods
  std::tm tm_snapshot;
#if defined(WIN32)
  ::gmtime_s(&tm_snapshot, &time);  // thread-safe?
#else
  ::gmtime_r(&time, &tm_snapshot);     // POSIX
#endif
  return tm_snapshot;
}

std::string getTimeStr(const char* fmt, time_t time) {
  std::tm tm_snapshot;
  if (!time) {
    time = ::time(NULL);
  }
#if defined(_WIN32)
  localtime_s(&tm_snapshot, &time);  // thread-safe
#else
  localtime_r(&time, &tm_snapshot);    // POSIX
#endif
  char buffer[1024];
  auto success = std::strftime(buffer, sizeof(buffer), fmt, &tm_snapshot);
  if (0 == success) return std::string(fmt);
  return buffer;
}

ClockInterface* g_clock = nullptr;

ClockInterface* SetClockForTesting(ClockInterface* clock) {
  ClockInterface* prev = g_clock;
  g_clock = clock;
  return prev;
}

ClockInterface* GetClockForTesting() { return g_clock; }

#if defined(WINUWP)

namespace {

class TimeHelper final {
 public:
  TimeHelper(const TimeHelper&) = delete;

  // Resets the clock based upon an NTP server. This routine must be called
  // prior to the main system start-up to ensure all clocks are based upon
  // an NTP server time if NTP synchronization is required. No critical
  // section is used thus this method must be called prior to any clock
  // routines being used.
  static void SyncWithNtp(int64_t ntp_server_time_ms) {
    auto& singleton = Singleton();
    TIME_ZONE_INFORMATION time_zone;
    GetTimeZoneInformation(&time_zone);
    int64_t time_zone_bias_ns =
        dchecked_cast<int64_t>(time_zone.Bias) * 60 * 1000 * 1000 * 1000;
    singleton.app_start_time_ns_ =
        (ntp_server_time_ms - kNTPTimeToUnixTimeEpochOffset) * 1000000 -
        time_zone_bias_ns;
    singleton.UpdateReferenceTime();
  }

  // Returns the number of nanoseconds that have passed since unix epoch.
  static int64_t TicksNs() {
    auto& singleton = Singleton();
    int64_t result = 0;
    LARGE_INTEGER qpcnt;
    QueryPerformanceCounter(&qpcnt);
    result = dchecked_cast<int64_t>(
        (dchecked_cast<uint64_t>(qpcnt.QuadPart) * 100000 /
         dchecked_cast<uint64_t>(singleton.os_ticks_per_second_)) *
        10000);
    result = singleton.app_start_time_ns_ + result -
             singleton.time_since_os_start_ns_;
    return result;
  }

 private:
  TimeHelper() {
    TIME_ZONE_INFORMATION time_zone;
    GetTimeZoneInformation(&time_zone);
    int64_t time_zone_bias_ns =
        dchecked_cast<int64_t>(time_zone.Bias) * 60 * 1000 * 1000 * 1000;
    FILETIME ft;
    // This will give us system file in UTC format.
    GetSystemTimeAsFileTime(&ft);
    LARGE_INTEGER li;
    li.HighPart = ft.dwHighDateTime;
    li.LowPart = ft.dwLowDateTime;

    app_start_time_ns_ = (li.QuadPart - kFileTimeToUnixTimeEpochOffset) * 100 -
                         time_zone_bias_ns;

    UpdateReferenceTime();
  }

  static TimeHelper& Singleton() {
    static TimeHelper singleton;
    return singleton;
  }

  void UpdateReferenceTime() {
    LARGE_INTEGER qpfreq;
    QueryPerformanceFrequency(&qpfreq);
    os_ticks_per_second_ = dchecked_cast<int64_t>(qpfreq.QuadPart);

    LARGE_INTEGER qpcnt;
    QueryPerformanceCounter(&qpcnt);
    time_since_os_start_ns_ = dchecked_cast<int64_t>(
        (dchecked_cast<uint64_t>(qpcnt.QuadPart) * 100000 /
         dchecked_cast<uint64_t>(os_ticks_per_second_)) *
        10000);
  }

 private:
  static constexpr uint64_t kFileTimeToUnixTimeEpochOffset =
      116444736000000000ULL;
  static constexpr uint64_t kNTPTimeToUnixTimeEpochOffset = 2208988800000L;

  // The number of nanoseconds since unix system epoch
  int64_t app_start_time_ns_;
  // The number of nanoseconds since the OS started
  int64_t time_since_os_start_ns_;
  // The OS calculated ticks per second
  int64_t os_ticks_per_second_;
};

}  // namespace

void SyncWithNtp(int64_t time_from_ntp_server_ms) {
  TimeHelper::SyncWithNtp(time_from_ntp_server_ms);
}

#endif  // defined(WINUWP)

int64_t SystemTimeNanos() {
  int64_t ticks;
#if defined(__apple__)
  static mach_timebase_info_data_t timebase;
  if (timebase.denom == 0) {
    // Get the timebase if this is the first time we run.
    // Recommended by Apple's QA1398.
    if (mach_timebase_info(&timebase) != KERN_SUCCESS) {
      // Fatal
    }
  }
  // Use timebase to convert absolute time tick units into nanoseconds.
  const auto mul = [](uint64_t a, uint32_t b)->int64_t {
    assert(b != 0);
    assert(a <= std::numeric_limits<int64_t>::max() / b);
    LOG(ERROR) << "The multiplication " << a << " * " << b << " overflows";
    return static_cast<int64_t>(a * b);
  };
  ticks = mul(mach_absolute_time(), timebase.numer) / timebase.denom;
#elif defined(__linux__)
  struct timespec ts;
  // TODO(deadbeef): Do we need to handle the case when CLOCK_MONOTONIC is not
  // supported?
  clock_gettime(CLOCK_MONOTONIC, &ts);
  ticks = kNumNanosecsPerSec * static_cast<int64_t>(ts.tv_sec) +
          static_cast<int64_t>(ts.tv_nsec);
#elif defined(WINUWP)
  ticks = TimeHelper::TicksNs();
#elif defined(WIN32) || defined(WIN64)
  static volatile LONG last_timegettime = 0;
  static volatile int64_t num_wrap_timegettime = 0;
  volatile LONG* last_timegettime_ptr = &last_timegettime;
  DWORD now = timeGetTime();
  // Atomically update the last gotten time
  DWORD old = InterlockedExchange(last_timegettime_ptr, now);
  if (now < old) {
    // If now is earlier than old, there may have been a race between threads.
    // 0x0fffffff ~3.1 days, the code will not take that long to execute
    // so it must have been a wrap around.
    if (old > 0xf0000000 && now < 0x0fffffff) {
      num_wrap_timegettime++;
    }
  }
  ticks = now + (num_wrap_timegettime << 32);
  // TODO(deadbeef): Calculate with nanosecond precision. Otherwise, we're
  // just wasting a multiply and divide when doing Time() on Windows.
  ticks = ticks * kNumNanosecsPerMillisec;
#else
#error Unsupported platform.
#endif
  return ticks;
}

int64_t SystemTimeMillis() {
  return static_cast<int64_t>(SystemTimeNanos() / kNumNanosecsPerMillisec);
}

int64_t TimeNanos() {
  if (g_clock) {
    return g_clock->TimeNanos();
  }
  return SystemTimeNanos();
}

uint32_t Time32() {
  return static_cast<uint32_t>(TimeNanos() / kNumNanosecsPerMillisec);
}

int64_t TimeMillis() { return TimeNanos() / kNumNanosecsPerMillisec; }

int64_t TimeMicros() { return TimeNanos() / kNumNanosecsPerMicrosec; }

int64_t TimeAfter(int64_t elapsed) {
  assert(elapsed >= 0);
  return TimeMillis() + elapsed;
}

int32_t TimeDiff32(uint32_t later, uint32_t earlier) { return later - earlier; }

int64_t TimeDiff(int64_t later, int64_t earlier) { return later - earlier; }

TimestampWrapAroundHandler::TimestampWrapAroundHandler()
    : last_ts_(0), num_wrap_(-1) {}

int64_t TimestampWrapAroundHandler::Unwrap(uint32_t ts) {
  if (num_wrap_ == -1) {
    last_ts_ = ts;
    num_wrap_ = 0;
    return ts;
  }

  if (ts < last_ts_) {
    if (last_ts_ >= 0xf0000000 && ts < 0x0fffffff) ++num_wrap_;
  } else if ((ts - last_ts_) > 0xf0000000) {
    // Backwards wrap. Unwrap with last wrap count and don't update last_ts_.
    return ts + ((num_wrap_ - 1) << 32);
  }

  last_ts_ = ts;
  return ts + (num_wrap_ << 32);
}

int64_t TmToSeconds(const tm& tm) {
  static short int mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  static short int cumul_mdays[12] = {0,   31,  59,  90,  120, 151,
                                      181, 212, 243, 273, 304, 334};
  int year = tm.tm_year + 1900;
  int month = tm.tm_mon;
  int day = tm.tm_mday - 1;  // Make 0-based like the rest.
  int hour = tm.tm_hour;
  int min = tm.tm_min;
  int sec = tm.tm_sec;

  bool expiry_in_leap_year =
      (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));

  if (year < 1970) return -1;
  if (month < 0 || month > 11) return -1;
  if (day < 0 || day >= mdays[month] + (expiry_in_leap_year && month == 2 - 1))
    return -1;
  if (hour < 0 || hour > 23) return -1;
  if (min < 0 || min > 59) return -1;
  if (sec < 0 || sec > 59) return -1;

  day += cumul_mdays[month];

  // Add number of leap days between 1970 and the expiration year, inclusive.
  day += ((year / 4 - 1970 / 4) - (year / 100 - 1970 / 100) +
          (year / 400 - 1970 / 400));

  // We will have added one day too much above if expiration is during a leap
  // year, and expiration is in January or February.
  if (expiry_in_leap_year && month <= 2 - 1)  // |month| is zero based.
    day -= 1;

  // Combine all variables into seconds from 1970-01-01 00:00 (except |month|
  // which was accumulated into |day| above).
  return (((static_cast<int64_t>(year - 1970) * 365 + day) * 24 + hour) * 60 +
          min) *
             60 +
         sec;
}

int64_t TimeUTCMicros() {
  if (g_clock) {
    return g_clock->TimeNanos() / kNumNanosecsPerMicrosec;
  }
#if defined(__linux__)
  struct timeval time;
  gettimeofday(&time, nullptr);
  // Convert from second (1.0) and microsecond (1e-6).
  return (static_cast<int64_t>(time.tv_sec) * kNumMicrosecsPerSec +
          time.tv_usec);

#elif defined(WIN32) || defined(WIN64) || defined(__CYGWIN__) || \
    defined(__CYGWIN32__)
  struct _timeb time;
  _ftime(&time);
  // Convert from second (1.0) and milliseconds (1e-3).
  return (static_cast<int64_t>(time.time) * kNumMicrosecsPerSec +
          static_cast<int64_t>(time.millitm) * kNumMicrosecsPerMillisec);
#endif
}

int64_t TimeUTCMillis() { return TimeUTCMicros() / kNumMicrosecsPerMillisec; }

}  // namespace MSF