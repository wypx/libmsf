
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
#include "acceptor.h"

#include <base/logging.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include "gcc_attr.h"
#include "event_loop.h"

using namespace MSF;

namespace MSF {

Acceptor::Acceptor(EventLoop *loop, const InetAddress &addr,
                   const NewConnCallback &cb)
    : loop_(loop), addr_(addr), new_conn_cb_(std::move(cb)) {}

Acceptor::~Acceptor() {
  if (idle_fd_ > 0) {
    CloseSocket(idle_fd_);
    idle_fd_ = kInvalidSocket;
  }
  if (sfd_ > 0) {
    CloseSocket(sfd_);
    sfd_ = kInvalidSocket;
  }
}

bool Acceptor::Start() {
  uint16_t event = EPOLLIN | EPOLLERR | EPOLLRDHUP;

/* 解决Accept惊群 */
#ifdef EPOLLEXCLUSIVE
  event |= EPOLLEXCLUSIVE;
#endif

  if (addr_.family() == AF_INET || addr_.family() == AF_INET6) {
    if (addr_.proto() == SOCK_STREAM) {
      LOG(INFO) << "host: " << addr_.host() << " port: " << addr_.port();
      sfd_ = CreateTcpServer(addr_.host(), addr_.port(), SOCK_STREAM, 5);
      if (sfd_ < 0) {
        return false;
      }
    } else if (addr_.proto() == SOCK_DGRAM) {
      sfd_ = CreateTcpServer(addr_.host(), addr_.port(), SOCK_DGRAM, 5);
      if (sfd_ < 0) {
        return false;
      }
    }
  } else if (addr_.family() == AF_UNIX) {
    // sfd_ = CreateUnixServer(addr_.host(), 5, 0777);
    if (sfd_ < 0) {
      return false;
    }
  } else {
    LOG(ERROR) << "not support this family type: " << addr_.family();
    return false;
  }
  if (::listen(sfd_, back_log_) < 0) {
    LOG(ERROR) << "failed to open listen for fd: " << sfd_;
    return false;
  }
  OpenIdleFd();

  return true;
}

void Acceptor::Stop() {
  loop_->RunInLoop([this] {
    CloseAccept();
    CloseIdleFd();
  });
}

void Acceptor::OpenListen() {
  loop_->RunInLoop([this] {
    if (::listen(sfd_, back_log_) < 0) {
      LOG(ERROR) << "failed to open listen for fd: " << sfd_;
    }
  });
}

void Acceptor::CloseListen() {
  loop_->RunInLoop([this] {
    if (::listen(sfd_, 0) < 0) {
      LOG(ERROR) << "failed to close listen for fd: " << sfd_;
    }
  });
}

void Acceptor::CloseAccept() {
  LOG(WARNING) << "acceptor close for fd: " << sfd_;
  if (sfd_ > 0) {
    CloseSocket(sfd_);
    sfd_ = kInvalidSocket;
  }
  new_conn_cb_ = nullptr;
}

void Acceptor::CloseIdleFd() {
  if (idle_fd_ > 0) {
    CloseSocket(idle_fd_);
    idle_fd_ = kInvalidSocket;
  }
}

void Acceptor::OpenIdleFd() {
  idle_fd_ = OpenDevNull(false);
  if (idle_fd_ < 0) {
    LOG(ERROR) << "open idle fd failed, errno: " << errno;
  }
}

// Receive accept queue
void Acceptor::PopAcceptQueue() { idle_fd_ = ::accept(sfd_, nullptr, nullptr); }

/* Close this connection immediately, indicating
 * that the server no longer provides services
 * Because the system resources are insufficient,
 * the server uses this method to reject the client's connection */
void Acceptor::DiscardAccept() {
  CloseIdleFd();
  PopAcceptQueue();
  CloseIdleFd();
  OpenIdleFd();
}

bool Acceptor::HandleError(int error) {
  switch (error) {
    case EINTR:
    case EPROTO:
    case ECONNABORTED: {
      LOG(WARNING) << "should retry accept, errno: " << error;
      return false;
    }
    case EAGAIN:
#if defined(WIN32) || defined(WIN64)
    case EWOULDBLOCK:
#endif
    {
      LOG(ERROR) << "should dicard accept, errno: " << error;
      return true;
    }
    case ENOBUFS:
    case ENOMEM: {
      LOG(ERROR) << "should dicard accept, errno: " << error;
      return true;
    }
    case EMFILE:
    case ENFILE: {
      // http://blog.qiusuo.im/blog/2014/04/28/epoll-emfile/
      //系统的上限ENFILE 进程的上限EMFILE
      LOG(ERROR) << "too many open connections, give up accept.";
      /* Read the section named "The special problem of
      * accept()ing when you can't" in libev's doc.
      * By Marc Lehmann, author of libev.*/
      DiscardAccept();
      CloseListen();
      return true;
    }
    default: {
      perror("accept()");
      return true;
    }
  };
}

/* Internal wrapper around 'accept' or 'accept4' to provide Linux-style
 * support for syscall-saving methods where available.
 *
 * In addition to regular accept behavior, you can set one or more of flags
 * SOCK_NONBLOCK and SOCK_CLOEXEC in the 'flags' argument, to make the
 * socket nonblocking or close-on-exec with as few syscalls as possible.
 */
void Acceptor::AcceptCb() {
  bool stop = false;
  int new_fd = -1;
  struct sockaddr_storage addr;
  socklen_t addrlen = sizeof(struct sockaddr_storage);

  do {
    ::memset(&addr, 0, sizeof(struct sockaddr_storage));
/* NOTE: we aren't using static inline function here; because accept4
 * requires defining _GNU_SOURCE and we don't want users to be forced to
 * define it in their application */
#if defined(HAVE_ACCEPT4) && defined(SOCK_CLOEXEC) && defined(SOCK_NONBLOCK)
    new_fd = ::accept4(_sfd_, (struct sockaddr *)&addr, addrlen, SOCK_NONBLOCK);
    if (new_fd >= 0 || (errno != EINVAL && errno != ENOSYS)) {
      /* A nonnegative result means that we succeeded, so return.
       * Failing with EINVAL means that an option wasn't supported,
       * and failing with ENOSYS means that the syscall wasn't
       * there: in those cases we want to fall back. Otherwise, we
       * got a real error, and we should return. */
      break;
    }
#endif
    /*
     * The returned socklen is ignored here, because sockaddr_in and
     * sockaddr_in6 socklens are not changed.  As to unspecified sockaddr_un
     * it is 3 byte length and already prepared, because old BSDs return zero
     * socklen and do not update the sockaddr_un at all; Linux returns 2 byte
     * socklen and updates only the sa_family part; other systems copy 3 bytes
     * and truncate surplus zero part.  Only bound sockaddr_un will be really
     * truncated here.
     */
    new_fd = ::accept(sfd_, (struct sockaddr *)&addr, &addrlen);
    if (unlikely(new_fd < 0)) {
      stop = HandleError(errno);
    } else {
      SetNonBlock(new_fd, true);
      /* 监听线程放在单独的线程, 如果想分发到线程池也可以*/
      new_conn_cb_(new_fd, EPOLLIN);
    }
  } while (stop);

  return;
}

}  // namespace MSF