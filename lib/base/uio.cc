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
#include "uio.h"

#include <errno.h>
#include <unistd.h>

using namespace MSF;

namespace MSF {

template <class F, class... Args>
static int wrapPositional(F f, int fd, off_t offset, Args... args) {
  off_t origLoc = lseek(fd, 0, SEEK_CUR);
  if (origLoc == off_t(-1)) {
    return -1;
  }
  if (lseek(fd, offset, SEEK_SET) == off_t(-1)) {
    return -1;
  }

  int res = (int)f(fd, args...);

  int curErrNo = errno;
  if (lseek(fd, origLoc, SEEK_SET) == off_t(-1)) {
    if (res == -1) {
      errno = curErrNo;
    }
    return -1;
  }
  errno = curErrNo;

  return res;
}

extern "C" ssize_t preadv(int fd, const iovec* iov, int count, off_t offset) {
  return wrapPositional(readv, fd, offset, iov, count);
}
extern "C" ssize_t pwritev(int fd, const iovec* iov, int count, off_t offset) {
  return wrapPositional(writev, fd, offset, iov, count);
}

}  // namespace MSF