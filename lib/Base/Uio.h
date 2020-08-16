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
#ifndef LIB_UIO_H_
#define LIB_UIO_H_

#include <limits.h>
#include <sys/types.h>
#include <sys/uio.h>

namespace MSF {
namespace IO {

extern "C" ssize_t preadv(int fd, const iovec* iov, int count, off_t offset);
extern "C" ssize_t pwritev(int fd, const iovec* iov, int count, off_t offset);

#ifdef _WIN32
extern "C" ssize_t readv(int fd, const iovec* iov, int count);
extern "C" ssize_t writev(int fd, const iovec* iov, int count);
#endif

constexpr size_t kIovMax = IOV_MAX;

}  // namespace IO
}  // namespace MSF
#endif
