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
#include "connector.h"

namespace MSF {

class FastServer {
 public:
  FastServer(EventLoop *loop, const InetAddress &addr);
  virtual ~FastServer();

  void StartAccept();
  void RestartAccept();
  void StopAccept();
  void QuitAccept();

  void SetCallback(const SuccCallback &scb, const ReadCallback &rcb,
                   const WriteCallback &wcb, const CloseCallback &ccb);

  virtual void ConnNewCallback(const int fd) = 0;

  virtual EventLoop *loop() { return acceptor_->loop(); }

 protected:
  virtual void ConnSuccCallback(const ConnectionPtr &conn);
  virtual void ConnReadCallback(const ConnectionPtr &conn);
  virtual void ConnWriteCallback(const ConnectionPtr &conn);
  virtual void ConnCloseCallback(const ConnectionPtr &conn);

  void AddConnection(const ConnectionPtr &conn) {
    connections_[conn->cid()] = conn;
  }
  void RemoveConnection(const ConnectionPtr &conn) {
    connections_.erase(conn->cid());
  }

  void AddConnector(const ConnectorPtr &ctor) {
    connectors_[ctor->conn()->cid()] = ctor;
  }
  void RemoveConnector(const ConnectorPtr &ctor) {
    connectors_.erase(ctor->conn()->cid());
  }

 protected:
  // max connection count to accept.
  // should >= -1, -1 means no limit, default -1.
  int max_connection_count_ = -1;

  // keep alive time of idle connections.
  // idle connections will be closed if no read/write for this time.
  // in seconds, should >= -1, -1 means forever, default -1.
  int keep_alive_time_;

  // Network throughput limit.
  // The network bandwidth is shared by all connections:
  // * busy connections get more bandwidth.
  // * the total bandwidth of all connections will not exceed the limit.
  int max_throughput_in_;       // max network in throughput for all connections.
                                // in MB/s, should >= -1, -1 means no limit, default -1.
  int max_throughput_out_;      // max network out throughput for all connections.
                                // in MB/s, should >= -1, -1 means no limit, default -1.

  //If disable Nagle's algorithm in tcp protocol.
  //default:true
  bool no_delay_; 

  SuccCallback scb_;
  ReadCallback rcb_;
  WriteCallback wcb_;
  CloseCallback ccb_;
  std::unique_ptr<Acceptor> acceptor_;
  std::unordered_map<uint64_t, ConnectionPtr> connections_;
  std::unordered_map<uint64_t, ConnectorPtr> connectors_;
};
}

#endif