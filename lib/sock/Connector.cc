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

Connector::~Connector() {
  close();
}

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

// void Connector::bufferReadCb() {
//   int ret;
//   int err;
//   rxIov_[0].iov_base = buffer;
//   rxIov_[0].iov_len = 4096;
//   while (true) {
//     ret = RecvMsg(fd_, &*rxIov_.begin(), rxIov_.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
//     MSF_DEBUG << "Recv ret:" << ret;
//     if (unlikely(ret == -1)) {
//       if (errno == EINTR) {
//         continue;
//       }
//       /* Maybe there is no data anymore */
//       if (errno == EAGAIN || errno == EWOULDBLOCK) {
//         MSF_DEBUG << "Recv no data";
//         break;
//       }
//       bufferCloseCb();
//       return;
//     } else if (ret == 0) {
//       close();
//       bufferCloseCb();
//       return;
//     }
//   }
//   ret = readable;
//   readCb_();
// }

// void Connector::bufferWriteCb() {
//   int ret;
//   int err;
//   while (true) {
//     ret = SendMsg(fd_, &*txIov_.begin(), txIov_.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
//     MSF_DEBUG << "Send ret:" << ret;
//     if (unlikely(ret == -1)) {
//       if (errno == EINTR) {
//         continue;
//       }
//       /* Maybe there is no data anymore */
//       if (errno == EAGAIN || errno == EWOULDBLOCK) {
//         break;
//       }
//       bufferCloseCb();
//       return;
//     } else if (ret == 0) {
//       bufferCloseCb();
//       return;
//     }
//   }
//   writeCb_();
// }

// void Connector::bufferCloseCb() {
//   close();
//   closeCb_();
// }

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