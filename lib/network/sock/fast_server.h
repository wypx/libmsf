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

#include <unordered_map>

#include "callback.h"
#include "acceptor.h"
#include "connection.h"

namespace MSF {

class FastServer {
 public:
  FastServer(EventLoop *loop, const InetAddress &addr);
  virtual ~FastServer();

  void StartAccept();
  void RestartAccept();
  void StopAccept();
  void QuitAccept();

  void SetConnSuccessCb(const ConnSuccCallback &cb) noexcept {
    succ_cb_ = std::move(cb);
  }
  void SetConnReadCb(const ReadCallback &cb) noexcept {
    read_cb_ = std::move(cb);
  }
  void SetConnWriteCb(const WriteCallback &cb) noexcept {
    write_cb_ = std::move(cb);
  }
  void SetConnCloseCb(const CloseCallback &cb) noexcept {
    close_cb_ = std::move(cb);
  }

  virtual void NewConnCallback(const int fd, const uint16_t event) = 0;

  EventLoop *loop() { return loop_; }

 protected:
  void ConnReadCallback(const ConnectionPtr &conn);
  void ConnWriteCallback(const ConnectionPtr &conn);
  void ConnCloseCallback(const ConnectionPtr &conn);

 protected:
  EventLoop *loop_;
  std::unique_ptr<Acceptor> acceptor_;
  std::unordered_map<uint64_t, ConnectionPtr> connections_;
  std::unordered_map<uint64_t, ConnectorPtr> connectors_;

  ConnSuccCallback succ_cb_;
  CloseCallback close_cb_;
  WriteCallback write_cb_;
  ReadCallback read_cb_;
};
}

#endif