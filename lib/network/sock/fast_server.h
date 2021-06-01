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
#ifndef SOCK_SERVER_H_
#define SOCK_SERVER_H_

#include "callback.h"
#include "acceptor.h"

namespace MSF {

class FastServer {
 public:
  FastServer(EventLoop *loop, const InetAddress &addr);
  virtual ~FastServer();

  void StartAccept();
  void RestartAccept();
  void StopAccept();
  void QuitAccept();

  virtual void NewConnCallback(const int fd, const uint16_t event) = 0;
  virtual void ConnReadCallback(const ConnectionPtr &conn) = 0;
  virtual void ConnWriteCallback(const ConnectionPtr &conn) = 0;
  virtual void ConnErrorCallback(const ConnectionPtr &conn) = 0;

 private:
  std::unique_ptr<Acceptor> acceptor_;
};
}

#endif