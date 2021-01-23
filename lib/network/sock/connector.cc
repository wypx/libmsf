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
#include "connector.h"
#include "sock_utils.h"
#include <butil/logging.h>

namespace MSF {

Connector::Connector() {}

Connector::~Connector() { CloseConn(); }

bool Connector::Connect(const std::string &host, const uint16_t port,
                        const uint32_t proto) {
  fd_ = ConnectTcpServer(host.c_str(), port, proto);
  if (fd_ < 0) {
    return false;
  }
  return true;
}

bool Connector::Connect(const std::string &srvPath,
                        const std::string &cliPath) {
  fd_ = ConnectUnixServer(srvPath.c_str(), cliPath.c_str());
  if (fd_ < 0) {
    return false;
  }
  return true;
}

}  // namespace MSF