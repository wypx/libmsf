#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "time_stamp.h"
#include "time_zone.h"
#include "thread.h"
#include "logging.h"

using namespace MSF;

namespace MSF {

__thread char t_errnobuf[512];
__thread char t_time[64];
__thread time_t t_lastSecond;

// static const char* kLogColorNames[] = {
//     COLOR_GRAY, COLOR_GRAY, COLOR_GREEN,  COLOR_YELLOW,
//     COLOR_BLUE, COLOR_RED,  COLOR_PURPLE, COLOR_BROWN,
//     COLOR_BLUE, COLOR_CYAN, COLOR_GREEN,  COLOR_GRAY,
// };

// const char *GetLogColor(Logger::LogLevel level) {
//   return kLogColorNames[level];
// }

const char* strerror_tl(int savedErrno) {
  return ::strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
}

LogLevel initLogLevel() {
  if (::getenv("MSF_LOG_TRACE"))
    return TRACE;
  else if (::getenv("MSF_LOG_DEBUG"))
    return L_DEBUG;
  else
    return INFO;
}

LogLevel g_logLevel = initLogLevel();

const char* LogLevelName[NUM_LOG_LEVELS] = {"TRACE ", "DEBUG ", "INFO  ",
                                            "WARN  ", "ERROR ", "FATAL ", };

// helper class for known string length at compile time
class T {
 public:
  T(const char* str, unsigned len) : str_(str), len_(len) {
    assert(strlen(str) == len_);
  }

  const char* str_;
  const unsigned len_;
};

inline LogStream& operator<<(LogStream& s, T v) {
  s.append(v.str_, v.len_);
  return s;
}

void defaultOutput(const char* msg, int len) {
  size_t n = fwrite(msg, 1, len, stdout);
  // FIXME check n
  (void)n;
}

void defaultFlush() { fflush(stdout); }

Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;
TimeZone g_logTimeZone;

inline char* GetCachedBasename(const char* file) {
  // 这个应该是thread-safe的,原因是file在运行起来的时候就不会变化了,而且是一个常量
  static __thread char* basename_ = nullptr;
  static __thread char* fullname_ = nullptr;
  static __thread char* slash = nullptr;

  if (fullname_ == nullptr) {
    fullname_ = const_cast<char*>(file);
    slash = ::strrchr(fullname_, '/');
    basename_ = (slash != nullptr) ? slash + 1 : fullname_;
  }
  // const char* slash = ::strrchr(filename, '/');
  // if (slash) {
  //     data_ = slash + 1;
  //   }
  //   size_ = static_cast<int>(strlen(data_));
  // }

  return basename_;
}

Logger::Impl::Impl(LogLevel level, int savedErrno, const char* file, int line)
    : time_(TimeStamp::now()), stream_(), level_(level), line_(line) {
  basename_ = GetCachedBasename(file);
  formatTime();
  CurrentThread::tid();
  stream_ << T(CurrentThread::tidString(), CurrentThread::tidStringLength());
  stream_ << T(LogLevelName[level], 6);
  if (savedErrno != 0) {
    stream_ << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
  }
}

//  std::stringstream timeStr;
//  localtime_r(&t, &tm);
//   https://blog.csdn.net/weixin_30641999/article/details/98304978
//  https://blog.csdn.net/zhaoxiaofeng44/article/details/81168058
//   https://stackoverflow.com/questions/55875862/c17-purpose-of-stdfrom-chars-and-stdto-chars
//    C++17添加了更高效的数值和字符串转换函数, 在头文件<charconv>中,
//    这些函数内部没有内存申请开销,和区域无关(locale-independent) */
//   timeStr << std::setw(4) << std::setfill('0') << tm.tm_year + 1900 << "-"
//           << std::setw(2) << std::setfill('0') << tm.tm_mon + 1 << "-"
//           << std::setw(2) << std::setfill('0') << tm.tm_mday << " "
//           << std::setw(2) << std::setfill('0') << tm.tm_hour << ":"
//           << std::setw(2) << std::setfill('0') << tm.tm_min << ":"
//           << std::setw(2) << std::setfill('0') << tm.tm_sec << "."
//           << std::setw(6) << std::to_string(CurrentMilliTime()).substr(0, 6);

void Logger::Impl::formatTime() {
  int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();
  time_t seconds = static_cast<time_t>(microSecondsSinceEpoch /
                                       TimeStamp::kMicroSecondsPerSecond);
  int microseconds = static_cast<int>(microSecondsSinceEpoch %
                                      TimeStamp::kMicroSecondsPerSecond);
  if (seconds != t_lastSecond) {
    t_lastSecond = seconds;
    struct tm tm_time;
    if (g_logTimeZone.valid()) {
      tm_time = g_logTimeZone.toLocalTime(seconds);
    } else {
      ::gmtime_r(&seconds, &tm_time);  // FIXME TimeZone::fromUtcTime
    }

    int len =
        snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    assert(len == 17);
    (void)len;
  }

  if (g_logTimeZone.valid()) {
    Fmt us(".%06d ", microseconds);
    assert(us.length() == 8);
    stream_ << T(t_time, 17) << T(us.data(), 8);
  } else {
    // Fmt us(".%06dZ ", microseconds);
    // assert(us.length() == 9);
    // stream_ << T(t_time, 17) << T(us.data(), 9);

    // 这里是计算毫秒,但是在要求较高性能的时候可以忽略,现在考虑如何降低
    FmtMicroSeconds us(microseconds);
    assert(us.length() == 9);
    stream_ << T(t_time, 17) << T(us.data(), 9);
  }
}

void Logger::Impl::finish() {
  stream_ << " - " << basename_ << ':' << line_ << '\n';
}

Logger::Logger(const char* file, int line) : impl_(INFO, 0, file, line) {}

Logger::Logger(const char* file, int line, LogLevel level, const char* func)
    : impl_(level, 0, file, line) {
  impl_.stream_ << func << ' ';
}

Logger::Logger(const char* file, int line, LogLevel level)
    : impl_(level, 0, file, line) {}

Logger::Logger(const char* file, int line, bool toAbort)
    : impl_(toAbort ? FATAL : ERROR, errno, file, line) {}

Logger::~Logger() {
  impl_.finish();
  const LogStream::Buffer& buf(stream().buffer());
  g_output(buf.data(), buf.length());
  if (impl_.level_ == FATAL) {
    g_flush();
    // usleep(100000);
    abort();
  }
}

void Logger::setLogLevel(LogLevel level) { g_logLevel = level; }

void Logger::setOutput(OutputFunc out) { g_output = out; }

void Logger::setFlush(FlushFunc flush) { g_flush = flush; }

void Logger::setTimeZone(const TimeZone& tz) { g_logTimeZone = tz; }

}  // namespace MSF