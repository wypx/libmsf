#include "MTime.h"
// #include <butil/logging.h>

using namespace MSF;

namespace MSF {

static msf_atomic_t msf_time_lock;
static volatile uint32_t
    g_current_msec;  //格林威治时间1970年1月1日凌晨0点0分0秒到当前时间的毫秒数

static void msf_time_update(void) {
  time_t sec;
  uint32_t msec;
  struct timeval tv;

  if (!msf_trylock(&msf_time_lock)) {
    return;
  }

  gettimeofday(&tv, NULL);
  sec = tv.tv_sec;
  msec = tv.tv_usec / 1000;

  g_current_msec = (uint32_t)sec * 1000 + msec;

  /* 禁止编译器对后面的语句优化,如果没有这个限制,
   * 编译器可能将前后两部分代码合并,可能导致这6个时间更新出现间隔
   * 期间若被读取会出现时间不一致的情况*/
  msf_memory_barrier();

  msf_unlock(&msf_time_lock);

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

static inline int msf_clock_gettime(struct timespec *ts) {
  return clock_gettime(CLOCK_MONOTONIC, ts);
}

#define NS_PER_S 1000000000
void SetTimespecRelative(struct timespec *p_ts, long long msec) {
  struct timeval tv;

  gettimeofday(&tv, (struct timezone *)nullptr);

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
    err = nanosleep(&ts, &ts);
  } while (err < 0 && errno == EINTR);
}

uint64_t GetNanoTime() {
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return now.tv_sec * 1000000000LL + now.tv_nsec;
}

uint64_t CurrentMilliTime() {
#if _MSC_VER
  extern "C" std::uint64_t __rdtsc();
#pragma intrinsic(__rdtsc)
  return __rdtsc();
#elif __GNUC__ && (__i386__ || __x86_64__)
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
    return  std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
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
  //     << std::chrono::duration_cast<std::chrono::minutes>(end - start).count()
  //     << " minutes";
  std::chrono::duration<double, std::ratio<60, 1>> duration_min =
      std::chrono::duration_cast<
          std::chrono::duration<double, std::ratio<60, 1>>>(end - start);
  // LOG(INFO) << "cost " << duration_min.count() << " minutes";

  std::chrono::duration<double, std::ratio<1, 1>> duration_s(end - start);
  // LOG(INFO) << "cost " << duration_s.count() << " seconds";
  // LOG(INFO)
  //     << "cost "
  //     << std::chrono::duration_cast<std::chrono::seconds>(end - start).count()
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
  //           << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
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

std::string getTimeStr(const char *fmt,time_t time){
    std::tm tm_snapshot;
    if(!time){
        time = ::time(NULL);
    }
#if defined(_WIN32)
    localtime_s(&tm_snapshot, &time); // thread-safe
#else
    localtime_r(&time, &tm_snapshot); // POSIX
#endif
    char buffer[1024];
    auto success = std::strftime(buffer, sizeof(buffer), fmt, &tm_snapshot);
    if (0 == success)
        return std::string(fmt);
    return buffer;
}

}  // namespace MSF