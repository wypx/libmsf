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
#include "buffer.h"

#include <sys/socket.h>
#include <sys/uio.h>

namespace MSF {

ssize_t Buffer::ReadFd(int fd, int *savedErrno) {
  // saved an ioctl()/FIONREAD call to tell how much to read
  char extrabuf[65536];
  struct iovec vec[3];
  const size_t writeable = writeableBytes();
  uint32_t idx = 0;
  if (readerIndex_ <= writerIndex_) {
    if (readerIndex_ == 0) {  // 尾部要留空一个
      vec[idx].iov_base = &buffer_[writerIndex_];
      vec[idx].iov_len = capacity_ - 1 - writerIndex_;
    } else {  // 头部有空，尾部写满
      vec[idx].iov_base = &buffer_[writerIndex_];
      vec[idx].iov_len = capacity_ - writerIndex_;
      if (readerIndex_ > 1) {  // 头部大于一个空格，需要加上头部空的区域
        ++idx;
        vec[idx].iov_base = &buffer_[0];
        vec[idx].iov_len = readerIndex_ - 1;
      }
    }
  } else {
    vec[idx].iov_base = &buffer_[writerIndex_];
    vec[idx].iov_len = readerIndex_ - 1 - writerIndex_;
  }
  // when there is enough space in this buffer, don't read into extrabuf.
  // when extrabuf is used, we read 128k-1 bytes at most.
  if (writeable < sizeof extrabuf) {
    ++idx;
    vec[idx].iov_base = extrabuf;
    vec[idx].iov_len = sizeof extrabuf;
  }
  const ssize_t n = ::readv(fd, vec, idx + 1);
  if (n < 0) {
    *savedErrno = errno;
  } else if ((size_t)n <= writeable) {
    hasWritten(n);
  } else {
    hasWritten(writeable);
    append(extrabuf, n - writeable);
  }
  return n;
}

ssize_t Buffer::WriteFd(int fd) {
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
    vec[0].iov_len = capacity_ - readerIndex_;
    iovcnt = 1;
    if (writerIndex_ != 0) {
      vec[1].iov_base = &buffer_[0];
      vec[1].iov_len = writerIndex_;
      iovcnt = 2;
    }
  }
  const ssize_t n = ::writev(fd, vec, iovcnt);
  if (n > 0) {
    retrieve(n);
  }
  return n;
}

}  // namespace MSF