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
#ifndef LIB_LOG_FILE_H_
#define LIB_LOG_FILE_H_

#include <string>
#include <mutex>
#include <memory>

namespace MSF {

namespace FileUtil {
class AppendFile;
}

class LogFile {
 public:
  LogFile(const std::string& filePath, off_t rollSize = 2048 * 1000,
          bool threadSafe = true, int flushInterval = 3, int checkEveryN = 1024);
  ~LogFile();

  void append(const char* logline, int len);
  void flush();
  bool rollFile();

 private:
  void append_unlocked(const char* logline, int len);

  static std::string GetLogFileName(const std::string& basename, time_t* now);

  const std::string basename_;
  const int64_t roll_size_;
  const int checkEveryN_;
 
  int count_;

  const std::string file_path_;
  const int flush_interval_;

  int rollCnt_;

  std::unique_ptr<std::mutex> mutex_;
  time_t startOfPeriod_;
  time_t lastRoll_;
  time_t lastFlush_;
  std::unique_ptr<FileUtil::AppendFile> file_;

  const static int kRollPerSeconds_ = 60*60*24;
};

}  // namespace MSF

#endif