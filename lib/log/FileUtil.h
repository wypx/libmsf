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
#ifndef LIB_FILE_UTIL_H_
#define LIB_FILE_UTIL_H_

#include <string>

namespace MSF {

// For passing C-style string argument to a function.
class StringArg {
 public:
  StringArg(const char* str) : m_str(str) {}

  StringArg(const std::string& str) : m_str(str.c_str()) {}

  const char* c_str() const { return m_str; }

 private:
  const char* m_str;
};

namespace FileUtil {

class AppendFile {
 public:
  explicit AppendFile(StringArg filePath);

  ~AppendFile();

  void append(const char* logline, const size_t len);

  size_t write(const char* logline, const size_t len);

  void flush();

  off_t writtenBytes() const { return m_writtenBytes; }

 private:
  static const size_t kFileBufferSize = 4096;
  size_t write(const char* logline, int len);

  FILE* m_fp;
  off_t m_writtenBytes;
  char m_buffer[kFileBufferSize];
};
}

}  // namespace MSF
#endif