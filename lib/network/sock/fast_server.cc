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
#include "logging.h"

namespace MSF {

FastServer::FastServer(EventLoop *loop, const InetAddress &addr) {
  acceptor_ = std::make_unique<Acceptor>(
      loop, addr,
      std::bind(&FastServer::ConnNewCallback, this, std::placeholders::_1));
}

FastServer::~FastServer() {}

void FastServer::StartAccept() { acceptor_->Start(); }

void FastServer::StopAccept() { return acceptor_->Stop(); }

void FastServer::RestartAccept() { return acceptor_->OpenListen(); }

void FastServer::QuitAccept() { return acceptor_->CloseListen(); }

void FastServer::SetCallback(const SuccCallback &scb, const ReadCallback &rcb,
                             const WriteCallback &wcb,
                             const CloseCallback &ccb) {
  scb_ = scb;
  rcb_ = rcb;
  wcb_ = wcb;
  ccb_ = ccb;
}

void FastServer::ConnSuccCallback(const ConnectionPtr &conn) {
  LOG(INFO) << "conn succ: " << conn->peer_addr().IPPort2String();
  connections_[conn->cid()] = conn;
  if (scb_) scb_(conn);
}
void FastServer::ConnReadCallback(const ConnectionPtr &conn) {
  LOG(INFO) << "conn read: " << conn->peer_addr().IPPort2String();
  if (rcb_) rcb_(conn);
}

void FastServer::ConnWriteCallback(const ConnectionPtr &conn) {
  LOG(INFO) << "conn write: " << conn->peer_addr().IPPort2String();
  if (wcb_) wcb_(conn);
}

void FastServer::ConnCloseCallback(const ConnectionPtr &conn) {
  LOG(INFO) << "conn close: " << conn->peer_addr().IPPort2String();
  if (ccb_) ccb_(conn);
  connections_.erase(conn->cid());
}
}
