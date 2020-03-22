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
#ifndef __MSF_LOGGER_H__
#define __MSF_LOGGER_H__

#include <stdarg.h>
#include <string.h>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>

#include <base/Define.h>
#include <base/GccAttr.h>
#include <base/Noncopyable.h>

using namespace MSF::BASE;

namespace MSF {
namespace BASE {

#ifdef __cplusplus
extern "C" {
#endif

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

enum LogLevel { LEVEL_TRACE, LEVEL_DEBUG, LEVEL_INFO, LEVEL_WARN, LEVEL_ERROR, LEVEL_FATAL };

enum LogState { LOG_CLOSED, LOG_ERROR, LOG_OPENING, LOG_OPENED, LOG_ZIPING };

class LogStream : public std::enable_shared_from_this<LogStream> {
 public:
  typedef std::function<void(const char *buff, int len)> pfnLogWriteCb;

  explicit LogStream() = default;
  explicit LogStream(enum LogLevel level, const char *func, const char *file,
                     const int line) /*noexcept*/;
  virtual ~LogStream();

  virtual void registCb(pfnLogWriteCb wcb) { writeCb = std::move(wcb); }

  std::ostringstream &stream() { return fmtStr; }

 private:
  pfnLogWriteCb writeCb;
  std::ostringstream fmtStr;

  std::string CurrProcessToString();
  std::string CurrTimeToString(void);
  std::string LogFileNameToString(void);
  std::string GetLogColor(enum LogLevel level);

  LogStream(const LogStream &other) = delete;
  LogStream &operator=(const LogStream &other) = delete;
};

#include <mutex>
class Logger : public Noncopyable {
 private:
  static std::unique_ptr<Logger> g_Logger;

 public:
  // https://blog.csdn.net/xijiacun/article/details/71023777
  // https://www.tuicool.com/articles/QbmInyF
  // https://blog.csdn.net/u011726005/article/details/82356538
  /** Using singleton logger instance in one process */
  static Logger &getLogger() {
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [&]() { g_Logger.reset(new Logger()); });
    return *g_Logger;
  }

  static Logger &GetInstance() {
    static Logger intance;
    return intance;
  }
  ~Logger();

  const enum LogLevel logLevel() const {
    return iLogLevel;
  }
  void setLogLevel(enum LogLevel level);
  bool setLogFilePath(const std::string &logpath); /* Need to reopen log file*/
  bool init(const std::string &logpath);
  int writeFile(const char *info, int len);
  std::shared_ptr<LogStream> LogWrite(enum LogLevel loglevel, const char *func,
                                      const char *file, const int line);

 private:
  std::string pLogPath;
  int iLogFd;
  enum LogState iState;    /* log running state */
  enum LogLevel iLogLevel; /* min level of log */

  bool bEnable;
  bool bLogPrint;
  bool bLogFile;
  int uLogSize; /* current log file size */

  bool bLogZip;
  int uZipSize;
  bool uZipAlg; /* lzma,tar.gz,zip, etc */
  bool bEncrypt;
  bool uEncAlg; /* md5,sha1,hash, etc */

  bool bRemoteBackup;
  int uUploadProt;
  std::string pRemoteUrl;

  std::mutex stLogMutex;
  std::unique_lock<std::mutex> stLogLock;
  std::mutex mutex_;

  Logger() = default;

  bool openLogFile(const std::string &filename);
  int compressFile(void);
};

#define MSF_TRACE \
  Logger::getLogger().LogWrite(LEVEL_TRACE, MSF_FUNC_FILE_LINE)->stream()
#define MSF_DEBUG \
  Logger::getLogger().LogWrite(LEVEL_DEBUG, MSF_FUNC_FILE_LINE)->stream()
#define MSF_INFO \
  Logger::getLogger().LogWrite(LEVEL_INFO, MSF_FUNC_FILE_LINE)->stream()
#define MSF_WARN \
  Logger::getLogger().LogWrite(LEVEL_WARN, MSF_FUNC_FILE_LINE)->stream()
#define MSF_ERROR \
  Logger::getLogger().LogWrite(LEVEL_ERROR, MSF_FUNC_FILE_LINE)->stream()
#define MSF_FATAL \
  Logger::getLogger().LogWrite(LEVEL_FATAL, MSF_FUNC_FILE_LINE)->stream()

#ifdef __cplusplus
}
#endif

}  // namespace BASE
}  // namespace MSF
#endif
