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
#include <base/GccAttr.h>
#include <base/Logger.h>
#include <base/Time.h>
#include <base/mem/Mem.h>
#include <conf/Config.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/types.h> /* gettid */
#include <unistd.h>

#include <cassert>
#include <iomanip>
#include <thread>
#define gettid() syscall(__NR_gettid)

using namespace MSF::TIME;

namespace MSF {
namespace BASE {

// std::map<std::string, Logger::LogLevel> ;

static const std::string g_LogLevel[] = {"TRACE", "DEBUG", "INFO",
                                         "WARN",  "ERROR", "FATAL"};

static const std::string g_logcolor[] = {
    COLOR_GRAY, COLOR_GRAY, COLOR_GREEN,  COLOR_YELLOW,
    COLOR_BLUE, COLOR_RED,  COLOR_PURPLE, COLOR_BROWN,
    COLOR_BLUE, COLOR_CYAN, COLOR_GREEN,  COLOR_GRAY,
};

LogStream::~LogStream() {
  // std::cout << "MSF_LogStream is do destroyed." << std::endl;

  /* String data generated in the constructor,
   * that should be appended to the message before output. */
  fmtStr << COLOR_L_CYAN << " [pid:" << getpid() << ",tid:" << gettid() << "]"
         << COLOR_NONE << std::endl;

  std::cout << fmtStr.str();
  std::cout << std::flush;

  writeCb(fmtStr.str().c_str(), fmtStr.str().length());
}

LogStream::LogStream(enum LogLevel level, const char *func, const char *file,
                     const int line) {
  // std::cout << "MSF_LogStream is do start." << std::endl;
  fmtStr << GetLogColor(level) << "[" << CurrTimeToString() << "]"
         << "[" << std::setw(5) << std::setfill(' ')
         << std::setiosflags(std::ios::left) << g_LogLevel[level] << "]"
         << "[" << func << " " << file << ":" << line << "] " << COLOR_NONE;
}

std::string LogStream::CurrTimeToString(void) {
  std::stringstream timeStr;
  struct tm tm;
  time_t t;

  time(&t);
  localtime_r(&t, &tm);
  // https://blog.csdn.net/weixin_30641999/article/details/98304978
  // https://blog.csdn.net/zhaoxiaofeng44/article/details/81168058
  // https://stackoverflow.com/questions/55875862/c17-purpose-of-stdfrom-chars-and-stdto-chars
  /* C++17添加了更高效的数值和字符串转换函数, 在头文件<charconv>中,
   * 这些函数内部没有内存申请开销,和区域无关(locale-independent) */
  timeStr << std::setw(4) << std::setfill('0') << tm.tm_year + 1900 << "-"
          << std::setw(2) << std::setfill('0') << tm.tm_mon + 1 << "-"
          << std::setw(2) << std::setfill('0') << tm.tm_mday << " "
          << std::setw(2) << std::setfill('0') << tm.tm_hour << ":"
          << std::setw(2) << std::setfill('0') << tm.tm_min << ":"
          << std::setw(2) << std::setfill('0') << tm.tm_sec << "."
          << std::setw(6) << std::to_string(CurrentMilliTime()).substr(0, 6);

  return timeStr.str();
}

std::string LogStream::CurrProcessToString() {
  std::stringstream processStr;

  return processStr.str();
}

std::string LogStream::LogFileNameToString(void) {
  std::stringstream fileNameStr;
  time_t t;
  time(&t);

  // fileNameStr << pLogPath << "/" << CurrProcessToString();

  return fileNameStr.str();
}

std::string LogStream::GetLogColor(enum LogLevel level) {
  return g_logcolor[level];
}

/***************** Next Logger Functions ****************/
std::unique_ptr<Logger> Logger::g_Logger;
Logger::~Logger() {
  // std::cout << "MSF_LOG is do destroyed." << std::endl;

  if (unlikely(iLogFd < 0 || iLogFd == STDERR_FILENO)) {
    return;
  }
  close(iLogFd);
  iLogFd = MSF_INVALID_SOCKET;
}

void Logger::setLogLevel(enum LogLevel level) { iLogLevel = level; }

bool Logger::openLogFile(const std::string &filename) {
  iState = LOG_OPENING;
  if (bLogFile) {
    iLogFd = open(pLogPath.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0766);
    if (iLogFd < 0) {
      MSF_ERROR << "Fail to open \"" << pLogPath << "\".";
      if (filename.empty()) {
        iLogFd = STDERR_FILENO;
      }
      iState = LOG_CLOSED;
      return false;
    } else {
      iState = LOG_OPENED;

      /* 将STDOUT_FILENO和STDERR_FILENO也重定向到这个文件 */
      // dup2(iLogFd, STDOUT_FILENO);
      // dup2(iLogFd, STDERR_FILENO);
      MSF_DEBUG << "Open \"" << pLogPath << "\" sucessfully.";
    }
  }
  return true;
}

bool Logger::init(const std::string &logPath) {
  bLogPrint = true;
  bLogFile = true;
  iLogLevel = LEVEL_INFO;

  // stLogLock = std::unique_lock<std::mutex>(stLogMutex, std::defer_lock);
  std::lock_guard<std::mutex> lock(mutex_);
  pLogPath = logPath;

  assert(openLogFile(logPath));

  return true;
}

int Logger::compressFile(void) {
  if (unlikely(iLogFd < 0 || iLogFd == STDERR_FILENO)) {
    return MSF_OK;
  }

  if (bLogZip) {
    if (uLogSize >= 64 * 1024 * 1024) {
      iState = LOG_ZIPING;

      // log_zip();
      close(iLogFd);

      // if (rename(logfilename1, logfilename2)) {
      //     remove(logfilename2);
      //     rename(logfilename1,logfilename2);
      // }

      /* empty log file */
      ftruncate(iLogFd, 0);
      lseek(iLogFd, 0, SEEK_SET);
      iState = LOG_CLOSED;
    }
  }

  return MSF_OK;
}

int Logger::writeFile(const char *info, int len) {
  int iRet = MSF_ERR;

  // std::cout << "MSF_LOG LogWriteFile flag: " <<  bLogFile << ":" << iLogFd <<
  // std::endl;

  if (!bLogFile) {
    return MSF_OK;
  }

  if (unlikely(iState != LOG_OPENED || iLogFd < 0)) {
    std::cout << "Logfile not opened." << std::endl;

    if (iState == LOG_CLOSED) {
      init(pLogPath);
    }
    return MSF_ERR;
  }

  iRet = write(iLogFd, info, len);
  if (iRet != len) {
    std::cout << "Lseek write log errno:" << errno << std::endl;
    fsync(iLogFd);
    return MSF_ERR;
  }

  fsync(iLogFd);

  iRet = compressFile();
  if (iRet < 0) {
    std::cout << "Logfile compress failed." << std::endl;
  }

  std::cout << std::flush;
  return MSF_OK;
}

std::shared_ptr<LogStream> Logger::LogWrite(enum LogLevel level,
                                            const char *func, const char *file,
                                            const int line) {
  // stLogLock.lock();

  /* No need to print or write to logfile */
  if (unlikely(level < iLogLevel)) {
    // stLogLock.unlock();
    // return std::cout;
  }

  std::shared_ptr<LogStream> logFormat =
      std::make_shared<LogStream>(level, func, file, line);

  logFormat->registCb(std::bind(&Logger::writeFile, this, std::placeholders::_1,
                                std::placeholders::_2));
  return logFormat;
}

}  // namespace BASE
}  // namespace MSF