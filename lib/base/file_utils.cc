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
#include "file_utils.h"

#include <sys/file.h>
#include <unistd.h>

#include "file_details.h"
#include "Uio.h"

using namespace MSF;

namespace MSF {

int openNoInt(const char* name, int flags, mode_t mode) {
  return int(wrapNoInt(open, name, flags, mode));
}

static int filterCloseReturn(int r) {
  // Ignore EINTR.  On Linux, close() may only return EINTR after the file
  // descriptor has been closed, so you must not retry close() on EINTR --
  // in the best case, you'll get EBADF, and in the worst case, you'll end up
  // closing a different file (one opened from another thread).
  //
  // Interestingly enough, the Single Unix Specification says that the state
  // of the file descriptor is unspecified if close returns EINTR.  In that
  // case, the safe thing to do is also not to retry close() -- leaking a file
  // descriptor is definitely better than closing the wrong file.
  if (r == -1 && errno == EINTR) {
    return 0;
  }
  return r;
}

int closeNoInt(int fd) { return filterCloseReturn(::close(fd)); }

int fsyncNoInt(int fd) { return int(wrapNoInt(fsync, fd)); }

int dupNoInt(int fd) { return int(wrapNoInt(dup, fd)); }

int dup2NoInt(int oldfd, int newfd) {
  return int(wrapNoInt(dup2, oldfd, newfd));
}

int fdatasyncNoInt(int fd) {
#if defined(__APPLE__)
  return int(wrapNoInt(fcntl, fd, F_FULLFSYNC));
#elif defined(__FreeBSD__) || defined(_MSC_VER)
  return int(wrapNoInt(fsync, fd));
#else
  return int(wrapNoInt(fdatasync, fd));
#endif
}

int ftruncateNoInt(int fd, off_t len) {
  return int(wrapNoInt(ftruncate, fd, len));
}

int truncateNoInt(const char* path, off_t len) {
  return int(wrapNoInt(truncate, path, len));
}

int flockNoInt(int fd, int operation) {
  return int(wrapNoInt(flock, fd, operation));
}

int shutdownNoInt(int fd, int how) {
  //   return int(wrapNoInt(shutdown, fd, how));
  return 0;
}

ssize_t readNoInt(int fd, void* buf, size_t count) {
  return wrapNoInt(read, fd, buf, count);
}

ssize_t preadNoInt(int fd, void* buf, size_t count, off_t offset) {
  return wrapNoInt(pread, fd, buf, count, offset);
}

ssize_t readvNoInt(int fd, const iovec* iov, int count) {
  return wrapNoInt(readv, fd, iov, count);
}

ssize_t writeNoInt(int fd, const void* buf, size_t count) {
  return wrapNoInt(write, fd, buf, count);
}

ssize_t pwriteNoInt(int fd, const void* buf, size_t count, off_t offset) {
  return wrapNoInt(pwrite, fd, buf, count, offset);
}

ssize_t writevNoInt(int fd, const iovec* iov, int count) {
  return wrapNoInt(writev, fd, iov, count);
}

ssize_t readFull(int fd, void* buf, size_t count) {
  return wrapFull(read, fd, buf, count);
}

ssize_t preadFull(int fd, void* buf, size_t count, off_t offset) {
  return wrapFull(pread, fd, buf, count, offset);
}

ssize_t writeFull(int fd, const void* buf, size_t count) {
  return wrapFull(write, fd, const_cast<void*>(buf), count);
}

ssize_t pwriteFull(int fd, const void* buf, size_t count, off_t offset) {
  return wrapFull(pwrite, fd, const_cast<void*>(buf), count, offset);
}

ssize_t readvFull(int fd, iovec* iov, int count) {
  return wrapvFull(readv, fd, iov, count);
}

ssize_t preadvFull(int fd, iovec* iov, int count, off_t offset) {
  return wrapvFull(preadv, fd, iov, count, offset);
}

ssize_t writevFull(int fd, iovec* iov, int count) {
  return wrapvFull(writev, fd, iov, count);
}

ssize_t pwritevFull(int fd, iovec* iov, int count, off_t offset) {
  return wrapvFull(pwritev, fd, iov, count, offset);
}

}  // namespace MSF