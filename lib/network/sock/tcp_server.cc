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
#include "tcp_server.h"
#include "logging.h"

namespace MSF {

FastTcpServer::FastTcpServer(EventLoop *loop, const InetAddress &addr)
    : FastServer(loop, addr) {}

FastTcpServer::~FastTcpServer() {}

void FastTcpServer::NewConnCallback(const int fd, const uint16_t event) {
  LOG(INFO) << "new conn coming: " << fd;
  auto conn = std::make_shared<TCPConnection>(loop(), fd, event);
  connections_[conn->cid()] = conn;
  succ_cb_(conn);
}
}
