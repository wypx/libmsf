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
#include <base/Logger.h>
#include <sock/Connector.h>
#include <unistd.h>

namespace MSF {
namespace SOCK {

Connector::Connector() : event_(Event()) {}

Connector::~Connector() {}

bool Connector::connect(const std::string &host, const uint16_t port,
                        const uint32_t proto) {
  fd_ = ConnectTcpServer(host.c_str(), port, proto);
  if (fd_ < 0) {
    return false;
  }
  return true;
}

bool Connector::connect(const std::string &srvPath,
                        const std::string &cliPath) {
  fd_ = ConnectUnixServer(srvPath.c_str(), cliPath.c_str());
  if (fd_ < 0) {
    return false;
  }
  return true;
}

void Connector::bufferReadCb(const EventCallback &cb) {
  int ret;
  int err;
  while (true) {
    ret = readBuffer_.readFd(fd_, &err);
    if (unlikely(ret == -1)) {
      if (err == EINTR) {
        continue;
      }
      /* Maybe there is no data anymore */
      if (err == EAGAIN || err == EWOULDBLOCK) {
        break;
      }
    } else if (ret == 0) {
      close();//
    }
  }
  cb();
}

void Connector::bufferWriteCb(const EventCallback &cb) {
  int ret;
  int err;
  while (true) {
    ret = writeBuffer_.sendFd(fd_, &err);
    if (unlikely(ret == -1)) {
      if (err == EINTR) {
        continue;
      }
      /* Maybe there is no data anymore */
      if (err == EAGAIN || err == EWOULDBLOCK) {
        break;
      }
    } else if (ret == 0) {
      close();//
    }
  }
  cb();
}

void Connector::bufferCloseCb(const EventCallback &cb) {
  close();
  cb();
}

void Connector::close() {
  if (fd_ > 0) {
    MSF_INFO << "Close conn for fd: " << fd_;
    event_.remove();
    ::close(fd_);
    fd_ = -1;
  }
}

}  // namespace SOCK
}  // namespace MSF