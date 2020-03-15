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
#ifndef __MSF_Connector_H__
#define __MSF_Connector_H__

#include <base/mem/Buffer.h>
#include <event/Event.h>
#include <event/EventLoop.h>
#include <sock/Socket.h>

#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

using namespace MSF::SOCK;
using namespace MSF::EVENT;

namespace MSF {
namespace SOCK {

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
  CONN_STATE_DEFAULT = 1,
  CONN_STATE_UNAUTHORIZED = 2,
  CONN_STATE_AUTHORIZED = 3,
  CONN_STATE_LOGINED = 4,
  CONN_STATE_UPGRADE = 5,
  CONN_STATE_CLOSED = 6,
  CONN_STATE_BUTT
};

class Connector;
typedef std::shared_ptr<Connector> ConnectionPtr;

class Connector : public std::enable_shared_from_this<Connector> {
 public:
  Connector();
  virtual ~Connector();

  void close();
  bool connect(const std::string &host, const uint16_t port,
               const uint32_t proto);

  bool connect(const std::string &srvPath, const std::string &cliPath);

  /* Connector and Accpetor connection use common structure */
  void init(EventLoop *loop, const int fd = -1) {
    fd_ = fd;
    event_.init(loop, fd);
  }

  void setSuccCallback(const EventCallback &cb) { 
    event_.setSuccCallback(cb);
  }
  void setReadCallback(const EventCallback &cb) {
    event_.setReadCallback(cb);
    // readCb_ = std::move(cb);
    // event_.setReadCallback(std::bind(&Connector::bufferReadCb, this));
  }
  void setWriteCallback(const EventCallback &cb) {
    event_.setWriteCallback(cb);
    // writeCb_ = std::move(cb);
    // event_.setWriteCallback(std::bind(&Connector::bufferWriteCb, this));
  }
  void setCloseCallback(const EventCallback &cb) {
    event_.setCloseCallback(cb);
    // closeCb_ = std::move(cb);
    // event_.setCloseCallback(std::bind(&Connector::bufferCloseCb, this));
  }
  void setErrorCallback(const EventCallback &cb) {
    event_.setErrorCallback(cb);
    // errorCb_ = std::move(cb);
    // event_.setErrorCallback(std::bind(&Connector::bufferCloseCb, this));
  }

  void enableEvent() {
    event_.enableReading();
    event_.enableClosing();
  }

  void enableWriting() { event_.enableWriting(); }
  void disableWriting() { event_.disableWriting(); }

  const int fd() const { return fd_; }
  void setCid(const uint32_t cid) { cid_ = cid; }
  const uint32_t cid() const { return cid_; }

  // bool addBuffer(char *data, const uint32_t len)
  // {
  //   if (fd_ < 0) {
  //     MSF_INFO << "Conn has been closed, cannot send buffer";
  //     return false;
  //   }
  //   struct iovec iov = { data, len };
  //   txIov_.push_back(std::move(iov));
  // }
  // bool removeBuffer(void *data, const uint32_t len)
  // {
  //   return true;
  // }
  // struct iovec peekBuffer()
  // {
  //   return rxIov_.front();
  // }
  char * peekBuffer()
  {
    return buffer;
  }

  size_t readableBytes() { return readable; }

 private:
  void updateActiveTime();
  // /* Read all data to buffer ring */
  // void bufferReadCb();
  // /* Send all data in buffer ring */
  // void bufferWriteCb();
  // void bufferCloseCb();

 public:
  Event event_;
  EventCallback readCb_;
  EventCallback writeCb_;
  EventCallback errorCb_;
  EventCallback closeCb_;

  std::string name_;
  std::string key_; /*key is used to find conn by cid*/
  uint32_t cid_;    /* unique identifier given by agent server when login */
  int fd_;
  uint32_t state_;

  uint64_t lastActiveTime_;

  char buffer[4096];
  int readable;

  //不用处理发送和接收,这样处理更简单
  Buffer readBuffer_;
  Buffer writeBuffer_;

  // std::vector<struct iovec> rxIov_;
  struct iovec rxIov_[2]; 
  uint32_t rxWanted_; /* One len of head or body */
  uint32_t rxRecved_; /* One len of head or body has recv*/

  // std::vector<struct iovec> txIov_;
  struct iovec txIov_[2];
  uint32_t txWanted_;     /* Total len of head and body */
  uint32_t txSended_;     /* Total len of head and body has send */
};

}  // namespace SOCK
}  // namespace MSF
#endif
