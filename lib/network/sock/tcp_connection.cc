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
    : Connection(loop, fd, thread_safe), event_(loop, fd) {
  loop_->UpdateEvent(&event_);
}

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
  if (fd_ != kInvalidSocket) {
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

void *TCPConnection::Malloc(size_t len, size_t align) { return nullptr; }

void TCPConnection::Free(void *ptr) {}

bool TCPConnection::HandleReadEvent() {
  if (state_ != State::kStateConnected) {
    LOG(ERROR) << fd_ << " want to read but wrong state: " << state_;
    return false;
  }
  int saved_errno = 0;
  size_t ret = read_buf_.ReadFd(fd_, &saved_errno);
  if (unlikely(IsConnError(ret))) {
    set_state(State::kStateError);
    CloseConn();
    return false;
  }
  return true;
}

bool TCPConnection::HandleWriteEvent() {
  if (state_ != State::kStateConnected &&
      state_ != State::kStateCloseWaitWrite) {
    LOG(ERROR) << fd_ << " want to rw but wrong state: " << state_;
    return false;
  }

  UpdateWriteBusyIovecSafe();

  if (write_busy_queue_.empty()) {
    return true;
  }

  int64_t ret = SendMsg(fd_, &*write_busy_queue_.begin(),
                        write_busy_queue_.size(), MSG_NOSIGNAL | MSG_DONTWAIT);
  if (unlikely(IsConnError(ret))) {
    return false;
  }
  UpdateWriteBusyOffset(ret);

  if (write_cb_) write_cb_(shared_from_this());
  return true;
}

void TCPConnection::HandleErrorEvent() {
  CloseConn();
  close_cb_(shared_from_this());
}

void TCPConnection::HandleConnectedEvent() {}

}  // namespace MSF