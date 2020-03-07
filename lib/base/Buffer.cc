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
#include "Buffer.h"

#include <sock/Socket.h>
#include <sys/uio.h>

namespace MSF {
namespace BASE {

const char Buffer::kCRLF[] = "\r\n";

const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;

ssize_t Buffer::readFd(int fd, int* savedErrno) {
  // saved an ioctl()/FIONREAD call to tell how much to read
  char extrabuf[65536];
  struct iovec vec[2];
  const size_t writable = writableBytes();
  vec[0].iov_base = begin() + writerIndex_;
  vec[0].iov_len = writable;
  vec[1].iov_base = extrabuf;
  vec[1].iov_len = sizeof extrabuf;
  // when there is enough space in this buffer, don't read into extrabuf.
  // when extrabuf is used, we read 128k-1 bytes at most.
  const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
  const ssize_t n = RecvMsg(fd, vec, iovcnt, MSG_NOSIGNAL | MSG_DONTWAIT);
  if (n < 0) {
    *savedErrno = errno;
  } else if (implicit_cast<size_t>(n) <= writable) {
    writerIndex_ += n;
  } else {
    writerIndex_ = buffer_.size();
    append(extrabuf, n - writable);
  }
  // if (n == writable + sizeof extrabuf)
  // {
  //   goto line_30;
  // }
  return n;
}

ssize_t Buffer::sendFd(const int fd, int* savedErrno) {
  struct iovec vec[2];
  uint32_t iovcnt;
  if (readerIndex_ < writerIndex_) {
    vec[0].iov_base = &buffer_[readerIndex_];
    vec[0].iov_len = writerIndex_ - readerIndex_;
    iovcnt = 1;
  } else if (readerIndex_ == writerIndex_) {
    return 0;
  } else {  // readerIndex_ > writerIndex_
    vec[0].iov_base = &buffer_[readerIndex_];
    vec[0].iov_len = internalCapacity() - readerIndex_;
    iovcnt = 1;
    if (writerIndex_ != 0) {
      vec[1].iov_base = &buffer_[0];
      vec[1].iov_len = writerIndex_;
      iovcnt = 2;
    }
  }
  const ssize_t n = SendMsg(fd, vec, iovcnt, MSG_NOSIGNAL | MSG_DONTWAIT);
  if (n <= 0) {
    *savedErrno = errno;
  } else {
    retrieve(n);
  }
  return n;
}

}  // namespace BASE
}  // namespace MSF