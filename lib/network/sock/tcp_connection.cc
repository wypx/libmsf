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
#include "tcp_connection.h"
#include "sock_utils.h"
#include "gcc_attr.h"
#include "event_loop.h"

#include <base/logging.h>

namespace MSF {

TCPConnection::TCPConnection(EventLoop *loop, int fd, bool thread_safe)
    : Connection(loop, fd, thread_safe) {}

TCPConnection::~TCPConnection() { CloseConn(); }

inline bool TCPConnection::IsConnError(int64_t ret) {
  return ((ret == 0) || (ret < 0 && errno != EINTR && errno != EAGAIN &&
                         errno != EWOULDBLOCK));
}

void TCPConnection::Shutdown(ShutdownMode mode) {
#if 0
  switch (mode) {
  case ShutdownMode::kShutdownRead:
      Shutdown(fd_, SHUT_RD);
      break;

  case ShutdownMode::kShutdownWrite:
      if (!write_buf_.Empty()) {
          LOG(WARNING) << fd_ << " shutdown write, but still has data to send";
          write_buf_.Clear();
      }

      Shutdown(fd_, SHUT_WR);
      break;

  case ShutdownMode::kShutdownBoth:
      if (!write_buf_.Empty()) {
          LOG(WARNING) << fd_ << " shutdown both, but still has data to send";
          write_buf_.Clear();
      }
      Shutdown(fd_, SHUT_RDWR);
      break;
  }
#endif
}

void TCPConnection::CloseConn() {
  loop_->RunInLoop([this] {
    if (fd_ != kInvalidSocket) {
      RemoveAllEvent();
      LOG(INFO) << "close conn for fd: " << fd_;
      if (write_buf_.WriteableBytes() == 0) {
        set_state(State::kStateActiveClose);
        Shutdown(ShutdownMode::kShutdownBoth);  // Force send FIN
      } else {
        set_state(State::kStateCloseWaitWrite);
        Shutdown(ShutdownMode::kShutdownRead);  // disable read
      }

      // event_.remove();
      CloseSocket(fd_);
      fd_ = kInvalidSocket;
    }
  });
}

bool TCPConnection::IsClosed() { return fd_ == kInvalidSocket; }

// recv in buffer
size_t TCPConnection::ReadableLength() { return read_buf_.ReadableBytes(); }

size_t TCPConnection::ReceiveData(void *data, size_t len) {
  return read_buf_.Receive(data, len);
}

size_t TCPConnection::DrainData(size_t len) { return read_buf_.Retrieve(len); }

size_t TCPConnection::RemoveData(void *data, size_t len) {
  return read_buf_.Remove(data, len);
}

void *TCPConnection::Malloc(size_t len, size_t align) {
  return loop_->Malloc(len);
}

void TCPConnection::Free(void *ptr) { loop_->Free(ptr); }

void TCPConnection::HandleReadEvent() {
  if (state_ != State::kStateConnected) {
    LOG(WARNING) << fd_ << " want to read but wrong state: " << state_;
    return;
  }
  int saved_errno = 0;
  size_t ret = read_buf_.ReadFd(fd_, &saved_errno);
  LOG(INFO) << "read ret: " << ret;
  if (unlikely(IsConnError(ret))) {
    set_state(State::kStateError);
    CloseConn();
    return;
  }
  return;
}

void TCPConnection::HandleWriteEvent() {
  if (state_ != State::kStateConnected &&
      state_ != State::kStateCloseWaitWrite) {
    LOG(WARNING) << fd_ << " want to rw but wrong state: " << state_;
    return;
  }

  if (UpdateWriteBusyIovecSafe()) {
    return;
  }

  int64_t ret = SendMsg(fd_, &*write_busy_queue_.begin(),
                        write_busy_queue_.size(), MSG_NOSIGNAL | MSG_DONTWAIT);
  if (unlikely(IsConnError(ret))) {
    return;
  }
  for (auto &iov : write_busy_queue_) {
    Free(iov.iov_base);
  }

  write_busy_queue_.clear();
  
  UpdateWriteBusyOffset(ret);

  RemoveWriteEvent();

  return;
}

void TCPConnection::HandleCloseEvent() {
  LOG(WARNING) << fd_ << " do close enent";
  CloseConn();
}

void TCPConnection::HandleSuccEvent() {
  LOG(WARNING) << fd_ << " do close enent";
}

}  // namespace MSF