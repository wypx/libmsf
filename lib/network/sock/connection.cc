
#include "connection.h"
#include "sock_utils.h"

#include <butil/logging.h>

using namespace MSF;

namespace MSF {

Connection::Connection() {}

Connection::~Connection() {
  Shutdown(ShutdownMode::SHUTDOWN_BOTH);  // Force send FIN
}

bool Connection::HandleReadEvent() {
#if 0
  int bytes = ::recv(fd_, recvBuf_.WriteAddr(), recvBuf_.WritableSize(), 0);
  if (bytes == kError) {
      if (EAGAIN == errno || EWOULDBLOCK == errno)
          return true;

      if (EINTR == errno)
          continue; // restart ::recv

      LOG(WARNING) << fd_ << " HandleReadEvent Error " << errno;
      Shutdown(ShutdownMode::SHUTDOWN_Both);
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
          Shutdown(ShutdownMode::SHUTDOWN_Read);
          // loop_->Modify(eET_Write, shared_from_this()); // disable read
      }

      return false;
  }
#endif
  return true;
}

bool Connection::HandleWriteEvent() {
  if (state_ != State::kStateConnected &&
      state_ != State::kStateCloseWaitWrite) {
    LOG(ERROR) << fd_ << " HandleWriteEvent wrong state " << state_;
    return false;
  }

  return true;
}

void Connection::HandleErrorEvent() { return; }

void Connection::Shutdown(ShutdownMode mode) {
#if 0
  switch (mode) {
  case ShutdownMode::SHUTDOWN_READ:
      Shutdown(fd_, SHUT_RD);
      break;

  case ShutdownMode::SHUTDOWN_WRITE:
      if (!send_buf_.Empty()) {
          LOG(WARNING) << fd_ << " shutdown write, but still has data to send";
          send_buf_.Clear();
      }

      Shutdown(fd_, SHUT_WR);
      break;

  case ShutdownMode::SHUTDOWN_BOTH:
      if (!send_buf_.Empty()) {
          LOG(WARNING) << fd_ << " shutdown both, but still has data to send";
          send_buf_.Clear();
      }
      Shutdown(fd_, SHUT_RDWR);
      break;
  }
#endif
}

void Connection::ActiveClose() {
#if 0
   if (fd_ == kInvalidSocket)
      return;

    if (send_buf_.Empty()) {
        Shutdown(ShutdownMode::SHUTDOWN_BOTH);
        state_ = State::kStateActiveClose;
    } else {
        state_ = State::kStateCloseWaitWrite;
        Shutdown(ShutdownMode::SHUTDOWN_READ); // disable read
    }

    // loop_->Modify(eET_Write, shared_from_this());
#endif
}

void Connection::CloseConn() {
  if (fd_ > 0) {
    LOG(INFO) << "Close conn for fd: " << fd_;
    // event_.remove();
    CloseSocket(fd_);
    fd_ = -1;
  }
}

}  // namespace MSF