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
#ifndef BASE_EXCEPTION_H_
#define BASE_EXCEPTION_H_

#include <iostream>
#include <errno.h>
#include <string.h>

#include "gcc_attr.h"

namespace MSF {

class Exception : public std::exception {
 public:
  Exception(std::string what);
  ~Exception() noexcept override = default;

  // default copy-ctor and operator= are okay.
  const char* what() const noexcept override { return message_.c_str(); }

  const char* stackTrace() const noexcept { return stack_.c_str(); }

  /** Dump current stack trace to a stream.
    * Thanks to http://stackoverflow.com/questions/77005.
     * \code
    * #include <iostream>
    * std::cerr << zypp::dumpBacktrace << std::endl;
    * \endcode
    * \code
    * #include <zypp/base/String.h>
    * std::string trace( str::Str() << zypp::dumpBacktrace );
    * \endcode
    */
  std::ostream& DumpBacktrace(std::ostream& stream_r);

  static int GetLastSystemError() __attribute_unused__;
  static std::string SystemErrorDescription(int error) __attribute_unused__;

 private:
  std::string message_;
  std::string stack_;
};

}  // namespace MSF
#endif