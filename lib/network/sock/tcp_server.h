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
#ifndef SOCK_TCP_SERVER_H_
#define SOCK_TCP_SERVER_H_

#include "fast_server.h"
#include "tcp_connection.h"

namespace MSF {

class FastTcpServer : public FastServer {
 public:
  FastTcpServer(EventLoop *loop, const InetAddress &addr);
  virtual ~FastTcpServer();

 protected:
  void ConnNewCallback(const int fd) override;
};
}

#endif