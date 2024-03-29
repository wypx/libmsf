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
#ifndef SOCK_CONNECTOR_H_
#define SOCK_CONNECTOR_H_

#include "connection.h"
#include "event.h"

using namespace MSF;

namespace MSF {

// http://itindex.net/detail/53322-rpc-%E6%A1%86%E6%9E%B6-bangerlee
/**
 * load balance: retry for timeout
 * client maintain a star mark for server(ip and port)
 * when request fail or timeout, dec the star by one,
 * since the star eaual as zero, add the ip, port, time
 * into the local info, realse the restict some times later
 *
 * server info can upload to zookeeper, then share in cluster
 * when new a server node, can distribute this to clients.
 */

enum ConnState {
  kCONN_STATE_DEFAULT = 1,
  kCONN_STATE_UNAUTHORIZED = 2,
  kCONN_STATE_AUTHORIZED = 3,
  kCONN_STATE_LOGINED = 4,
  kCONN_STATE_UPGRADE = 5,
  kCONN_STATE_CLOSED = 6,
  kCONN_STATE_BUTT
};

class Connector {
 public:
  Connector(EventLoop *loop, const InetAddress &peer);
  virtual ~Connector();

  bool Connect();

  // void SetSuccCallback(const ConnSuccCallback &cb) { conn_.SetConnSuccCb(cb);
  // }
  // void SetReadCallback(const ReadCallback &cb) { conn_.SetConnReadCb(cb); }
  // void SetWriteCallback(const WriteCallback &cb) {
  //   conn_.SetConnWriteCb(cb);
  // }
  // void SetCloseCallback(const CloseCallback &cb) {
  //   conn_.SetConnCloseCb(cb);
  // }

  void set_cid(const uint32_t cid) { cid_ = cid; }
  const uint32_t cid() const { return cid_; }

  const ConnectionPtr &conn() { return conn_; }

 protected:
  EventLoop *loop_;
  InetAddress peer_;
  ConnectionPtr conn_;
  std::string name_;
  std::string key_; /*key is used to find conn by cid*/
  uint32_t cid_;    /* unique identifier */
};

}  // namespace MSF
#endif
