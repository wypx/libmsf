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
#ifndef LIB_LOGGER_H_
#define LIB_LOGGER_H_

#include <string.h>
#include "log_stream.h"
#include "time_stamp.h"

using namespace MSF;

namespace MSF {

/* Log print colors */
#define COLOR_NONE "\e[0;m"
#define COLOR_BLACK "\e[0;30m"
#define COLOR_L_BLACK "\e[1;30m"
#define COLOR_RED "\e[0;31m"
#define COLOR_L_RED "\e[1;31m"
#define COLOR_GREEN "\e[0;32m"
#define COLOR_L_GREEN "\e[1;32m"
#define COLOR_BROWN "\e[0;33m"
#define COLOR_YELLOW "\e[1;33m"
#define COLOR_BLUE "\e[0;34m" /* "\033[0;34m" */
#define COLOR_L_BLUE "\e[1;34m"
#define COLOR_PURPLE "\e[0;35m"
#define COLOR_L_PURPLE "\e[1;35m"
#define COLOR_CYAN "\e[0;36m"
#define COLOR_L_CYAN "\e[1;36m"
#define COLOR_GRAY "\e[0;37m"
#define COLOR_WHITE "\e[1;37m"

#define COLOR_BOLD "\e[1m"
#define COLOR_UNDERLINE "\e[4m"
#define COLOR_BLINK "\e[5m"
#define COLOR_REVERSE "\e[7m"
#define COLOR_HIDE "\e[8m"
#define COLOR_CLEAR "\e[2J"
#define COLOR_CLRLINE "\r\e[K" /* or "\e[1K\r" */

// https://blog.csdn.net/mathgeophysics/article/details/9622761
// #define MSF_FUNC_FILE_LINE   __func__, __FILE__, __LINE__
#define MSF_FUNC_FILE_LINE __FUNCTION__, __FILE__, __LINE__

class TimeZone;

enum LogLevel {
  TRACE,
  L_DEBUG,
  INFO,
  WARNING,
  ERROR,
  FATAL,
  NUM_LOG_LEVELS,
};
class Logger {
 public:
  Logger(const char* file, int line);
  Logger(const char* file, int line, LogLevel level);
  Logger(const char* file, int line, LogLevel level, const char* func);
  Logger(const char* file, int line, bool toAbort);
  ~Logger();

  LogStream& stream() { return impl_.stream_; }

  static LogLevel logLevel();
  static void setLogLevel(LogLevel level);

  typedef void (*OutputFunc)(const char* msg, int len);
  typedef void (*FlushFunc)();
  static void setOutput(OutputFunc);
  static void setFlush(FlushFunc);
  static void setTimeZone(const TimeZone& tz);

 private:
  class Impl {
   public:
    Impl(LogLevel level, int old_errno, const char* file, int line);
    void formatTime();
    void finish();

    TimeStamp time_;
    LogStream stream_;
    LogLevel level_;
    int line_;
    const char* fullname_;
    const char* basename_;
  };

  Impl impl_;
};

extern LogLevel g_logLevel;

inline LogLevel Logger::logLevel() { return g_logLevel; }

//
// CAUTION: do not write:
//
// if (good)
//   LOG_INFO << "Good news";
// else
//   LOG_WARN << "Bad news";
//
// this expends to
//
// if (good)
//   if (logging_INFO)
//     logInfoStream << "Good news";
//   else
//     logWarnStream << "Bad news";
//
#define LOG_TRACE                  \
  if (Logger::logLevel() <= TRACE) \
  Logger(__FILE__, __LINE__, TRACE, __func__).stream()
#define LOG_DEBUG                    \
  if (Logger::logLevel() <= L_DEBUG) \
  Logger(__FILE__, __LINE__, L_DEBUG, __func__).stream()
#define LOG_INFO \
  if (Logger::logLevel() <= INFO) Logger(__FILE__, __LINE__).stream()
#define LOG_WARN Logger(__FILE__, __LINE__, WARN).stream()
#define LOG_ERROR Logger(__FILE__, __LINE__, ERROR).stream()
#define LOG_FATAL Logger(__FILE__, __LINE__, FATAL).stream()
#define LOG_SYSERR Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL Logger(__FILE__, __LINE__, true).stream()

#define LOG(level) \
  if (Logger::logLevel() <= level) Logger(__FILE__, __LINE__, level).stream()

const char* strerror_tl(int savedErrno);

// Taken from glog/logging.h
//
// Check that the input is non NULL.  This very useful in constructor
// initializer lists.

#define CHECK_NOTNULL(val) \
  ::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

// A small helper for CHECK_NOTNULL().
template <typename T>
T* CheckNotNull(const char* file, int line, const char* names, T* ptr) {
  if (ptr == NULL) {
    Logger(file, line, FATAL).stream() << names;
  }
  return ptr;
}

}  // namespace MSF

#endif