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
#include "fast_server.h"

namespace MSF {

FastServer::FastServer(EventLoop *loop, const InetAddress &addr) {
  acceptor_ = std::make_unique<Acceptor>(
      loop, addr, std::bind(&FastServer::NewConnCallback, this,
                            std::placeholders::_1, std::placeholders::_2));
}

FastServer::~FastServer() {}

void FastServer::StartAccept() { acceptor_->Start(); }

void FastServer::StopAccept() { return acceptor_->Stop(); }

void FastServer::RestartAccept() { return acceptor_->OpenListen(); }

void FastServer::QuitAccept() { return acceptor_->CloseListen(); }

void FastServer::ConnReadCallback(const ConnectionPtr &conn) { read_cb_(conn); }

void FastServer::ConnWriteCallback(const ConnectionPtr &conn) {
  write_cb_(conn);
}

void FastServer::ConnCloseCallback(const ConnectionPtr &conn) {
  close_cb_(conn);
  connections_.erase(conn->cid());
}
}
