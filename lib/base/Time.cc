#include <base/Time.h>

using namespace MSF::TIME;

namespace MSF {
namespace TIME {

static msf_atomic_t  msf_time_lock;
static volatile uint32_t g_current_msec; //格林威治时间1970年1月1日凌晨0点0分0秒到当前时间的毫秒数


static void msf_time_update(void) {

    time_t           sec;
    uint32_t         msec;
    struct timeval   tv;

    if (!msf_trylock(&msf_time_lock)) {
        return;
    }

    gettimeofday(&tv, NULL);
    sec = tv.tv_sec;
    msec = tv.tv_usec / 1000;

    g_current_msec = (uint32_t) sec * 1000 + msec;


    /* 禁止编译器对后面的语句优化,如果没有这个限制,
     * 编译器可能将前后两部分代码合并,可能导致这6个时间更新出现间隔
     * 期间若被读取会出现时间不一致的情况*/
    msf_memory_barrier();


    msf_unlock(&msf_time_lock);

    MSF_INFO << "Current time msec: " << g_current_msec;
}

static uint32_t msf_monotonic_time(time_t sec, uint32_t msec) {
#if (MSF_HAVE_CLOCK_MONOTONIC)
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

    return (uint32_t) sec * 1000 + msec;
}

static inline int msf_clock_gettime(struct timespec *ts) {
    return clock_gettime(CLOCK_MONOTONIC, ts);
}

#define NS_PER_S 1000000000
void SetTimespecRelative(struct timespec *p_ts, long long msec)
{
    struct timeval tv;

    gettimeofday(&tv, (struct timezone *) nullptr);

    p_ts->tv_sec = tv.tv_sec + (msec / 1000);
    p_ts->tv_nsec = (tv.tv_usec + (msec % 1000) * 1000L ) * 1000L;
    /* assuming tv.tv_usec < 10^6 */
    if (p_ts->tv_nsec >= NS_PER_S) {
        p_ts->tv_sec++;
        p_ts->tv_nsec -= NS_PER_S;
    }
}

void SleepMsec(long long msec)
{
    struct timespec ts;
    int err;

    ts.tv_sec = (msec / 1000);
    ts.tv_nsec = (msec % 1000) * 1000 * 1000;

    do {
        err = nanosleep (&ts, &ts);
    } while (err < 0 && errno == EINTR);
}

uint64_t GetNanoTime()
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec * 1000000000LL + now.tv_nsec;
}

uint64_t CurrentMilliTime()
{
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

  /* 
    auto tp = 
        std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    auto tmp = 
        std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
    return tmp.count();
  */
#endif
}

/**
 *  chrono库主要包含了三种类型: 时间间隔duration、时钟clock和时间点time_point
 *  https://blog.csdn.net/u013390476/article/details/50209603
 * 
 * */
inline void GetExecuteTime(ExecutorFunc f, void *arg)
{
    auto start = std::chrono::system_clock::now();
    f(arg);
    auto end = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
 
    std::cout   << "Cost time: " 
                << double(duration.count()) * std::chrono::microseconds::period::num /   
                std::chrono::microseconds::period::den 
                << "s" << std::endl;
}

void msf_time_init(void)
{

    struct timeval  tv;
    time_t          sec;
    uint32_t        msec;
    // struct tm       tm, gmt;

    gettimeofday(&tv, NULL);

    sec = tv.tv_sec;
    msec = tv.tv_usec / 1000;

    g_current_msec = msf_monotonic_time(sec, msec);

    msf_time_update();
}

} /**************************** end namespace BASE ****************************/
} /**************************** end namespace MSF  ****************************/