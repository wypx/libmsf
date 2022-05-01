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
#include "ssl_connection.h"

#include <base/logging.h>
#include <sys/poll.h>

#include "event_loop.h"
#include "gcc_attr.h"
#include "sock_utils.h"

namespace MSF {

SSL_CTX *sslContext;

bool GlobalSslInitialize() {
  SSL_load_error_strings();
  SSL_library_init();
  sslContext = SSL_CTX_new(SSLv23_client_method());
  if (!sslContext) {
    ERR_print_errors_fp(stderr);
    return false;
  }
  return true;
}

void GlobalSslDestroy() { SSL_CTX_free(sslContext); }

SSLConnection::SSLConnection(EventLoop *loop, int fd, bool thread_safe)
    : Connection(loop, fd, thread_safe) {}

SSLConnection::~SSLConnection() {
  if (ssl_handle_) {
    SSL_shutdown(ssl_handle_);
    SSL_free(ssl_handle_);
  }
  CloseConn();
}

inline bool SSLConnection::IsConnError(int64_t ret) {
  return ((ret == 0) || (ret < 0 && errno != EINTR && errno != EAGAIN &&
                         errno != EWOULDBLOCK));
}

void SSLConnection::Shutdown(ShutdownMode mode) {
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

void SSLConnection::CloseConn() {
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

bool SSLConnection::IsClosed() { return fd_ == kInvalidSocket; }

// recv in buffer
size_t SSLConnection::ReadableLength() { return read_buf_.ReadableBytes(); }

size_t SSLConnection::ReceiveData(void *data, size_t len) {
  return read_buf_.Receive(data, len);
}

size_t SSLConnection::DrainData(size_t len) { return read_buf_.Retrieve(len); }

size_t SSLConnection::RemoveData(void *data, size_t len) {
  return read_buf_.Remove(data, len);
}

void *SSLConnection::Malloc(size_t len, size_t align) {
  return loop_->Malloc(len);
}

void SSLConnection::Free(void *ptr) { loop_->Free(ptr); }

void SSLConnection::HandleReadEvent() {
  if (state_ != State::kStateConnected) {
    LOG(WARNING) << fd_ << " want to read but wrong state: " << state_;
    return;
  }
  int saved_errno = 0;
  if (!connected_) {
    ssl_handle_ = SSL_new(sslContext);
    if (!ssl_handle_) {
      ERR_print_errors_fp(stderr);
    }
    ssl_handle_ = SSL_new(sslContext);
    if (!ssl_handle_) {
      ERR_print_errors_fp(stderr);
    }

    if (!SSL_set_fd(ssl_handle_, fd_)) {
      ERR_print_errors_fp(stderr);
    }
    SSL_set_connect_state(ssl_handle_);

    int r = 0;
    int events = POLLIN | POLLOUT | POLLERR;
    while ((r = SSL_do_handshake(ssl_handle_)) != 1) {
      int err = SSL_get_error(ssl_handle_, r);
      if (err == SSL_ERROR_WANT_WRITE) {
        events |= POLLOUT;
        events &= ~POLLIN;
        LOG(WARNING) << "return want write set events:  " << events;
      } else if (err == SSL_ERROR_WANT_READ) {
        events |= EPOLLIN;
        events &= ~EPOLLOUT;
        LOG(INFO) << "return want read set events events:  " << events;
      } else {
        LOG(ERROR) << "SSL_do_handshake return " << r << " error " << err
                   << " errno " << errno;
        ERR_print_errors_fp(stderr);
        return;
      }
      struct pollfd pfd;
      pfd.fd = fd_;
      pfd.events = events;
      do {
        r = ::poll(&pfd, 1, 100);
      } while (r == 0);
      // check1(r == 1, "poll return %d error events: %d errno %d %s\n", r,
      // pfd.revents, errno, strerror(errno));
    }
    connected_ = true;
    LOG(INFO) << "ssl connected success";
  }

  // r = SSL_read(ssl_handle_, buf+rd, sizeof buf - rd);
  // if (r < 0) {
  //     int err = SSL_get_error(ssl_handle_, r);
  //     if (err == SSL_ERROR_WANT_READ) {
  //         continue;
  //     }
  //     ERR_print_errors_fp (stderr);
  // }

  size_t ret = read_buf_.ReadFd(fd_, &saved_errno);
  LOG(INFO) << "read ret: " << ret;
  if (unlikely(IsConnError(ret))) {
    set_state(State::kStateError);
    CloseConn();
    return;
  }
  return;
}

void SSLConnection::HandleWriteEvent() {
  if (state_ != State::kStateConnected &&
      state_ != State::kStateCloseWaitWrite) {
    LOG(WARNING) << fd_ << " want to rw but wrong state: " << state_;
    return;
  }

  if (UpdateWriteBusyIovecSafe()) {
    return;
  }

  // int wd = SSL_write(ssl_handle_, text, len);

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

void SSLConnection::HandleCloseEvent() {
  LOG(WARNING) << fd_ << " do close enent";
  CloseConn();
}

void SSLConnection::HandleSuccEvent() {
  LOG(WARNING) << fd_ << " do close enent";
}

}  // namespace MSF