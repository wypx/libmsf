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

void FastTcpServer::ConnNewCallback(const int fd) {
  LOG(INFO) << "new conn coming fd: " << fd;
  auto conn = std::make_shared<TCPConnection>(loop(), fd);

  static uint64_t g_conn_id = 0;
  conn->set_cid(g_conn_id++);
  conn->set_state(Connection::kStateConnected);
  conn->SetConnSuccCb(std::bind(&FastTcpServer::ConnSuccCallback, this, conn));
  conn->SetConnReadCb(std::bind(&FastTcpServer::ConnReadCallback, this, conn));
  conn->SetConnWriteCb(
      std::bind(&FastTcpServer::ConnWriteCallback, this, conn));
  conn->SetConnCloseCb(
      std::bind(&FastTcpServer::ConnCloseCallback, this, conn));
  conn->AddGeneralEvent();

  ConnSuccCallback(conn);
}
}  // namespace MSF
