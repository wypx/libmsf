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
#include "exception.h"

using namespace MSF;
namespace MSF {

Exception::Exception(std::string msg)
    : message_(std::move(msg)), stack_(stackTrace(/*demangle=*/)) {}

// https://www.cnblogs.com/xrcun/p/3210889.html
int Exception::GetLastSystemError() {
#if defined(WIN32) || defined(WIN64)
  return ::WSAGetLastError();
#else
  return errno;
#endif
}

static std::string SystemErrorDescription(int error) {
  const int capacity = 1024;
#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
  char buffer[capacity];
#if defined(__APPLE__) || defined(__CYGWIN__)
  int result = ::strerror_r(error, buffer, capacity);
  if (result != 0)
#else
  char* result = ::strerror_r(error, buffer, capacity);
  if (result == nullptr)
#endif
    return "Cannot convert the given system error code to the system message "
           "- " +
           std::to_string(error);
  else
    return std::string(buffer);
#elif defined(WIN32) || defined(WIN64)
  WCHAR buffer[capacity];
  DWORD size = ::FormatMessageW(
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
      error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, capacity,
      nullptr);
  return Encoding::ToUTF8((size >= 2) ? std::wstring(buffer, (size - 2))
                                      : std::wstring(L"Unknown system error!"));
#endif
}

}  // namespace MSF