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

#include <butil/logging.h>

namespace MSF {

TCPConnection::TCPConnection() {}

TCPConnection::~TCPConnection() {
  Shutdown(ShutdownMode::kShutdownBoth);  // Force send FIN
}

bool TCPConnection::HandleReadEvent() {
#if 0
  int bytes = ::recv(fd_, recvBuf_.WriteAddr(), recvBuf_.WritableSize(), 0);
  if (bytes == kError) {
      if (EAGAIN == errno || EWOULDBLOCK == errno)
          return true;

      if (EINTR == errno)
          continue; // restart ::recv

      LOG(WARNING) << fd_ << " HandleReadEvent Error " << errno;
      Shutdown(ShutdownMode::kShutdownBoth);
      state_ = State::kStateError;
      return false;
  }

  if (bytes == 0) {
      LOG(WARNING) << fd_ << " HandleReadEvent EOF ";
      if (sendBuf_.Empty()) {
          Shutdown(ShutdownMode::SHUTDOWN_Both);
          state_ = State::kStatePassiveClose;
      } else {
          state_ = State::kStateCloseWaitWrite;
          Shutdown(ShutdownMode::kShutdownRead);
          // loop_->Modify(eET_Write, shared_from_this()); // disable read
      }

      return false;
  }
#endif
  return true;
}

bool TCPConnection::HandleWriteEvent() {
  if (state_ != State::kStateConnected &&
      state_ != State::kStateCloseWaitWrite) {
    LOG(ERROR) << fd_ << " HandleWriteEvent wrong state " << state_;
    return false;
  }

  int ret = SendMsg(fd_, &*write_busy_queue_.begin(), write_busy_queue_.size(),
                    MSG_NOSIGNAL | MSG_DONTWAIT);

  return true;
}

void TCPConnection::HandleErrorEvent() { return; }

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

void TCPConnection::ActiveClose() {
#if 0
   if (fd_ == kInvalidSocket)
      return;

    if (write_buf_.Empty()) {
        Shutdown(ShutdownMode::kShutdownBoth);
        state_ = State::kStateActiveClose;
    } else {
        state_ = State::kStateCloseWaitWrite;
        Shutdown(ShutdownMode::kShutdownRead); // disable read
    }

    // loop_->Modify(eET_Write, shared_from_this());
#endif
}

void TCPConnection::CloseConn() {
  if (fd_ > 0) {
    LOG(INFO) << "close conn for fd: " << fd_;
    Shutdown(ShutdownMode::kShutdownBoth);  // Force send FIN
    // event_.remove();
    CloseSocket(fd_);
    fd_ = -1;
  }
}

}  // namespace MSF