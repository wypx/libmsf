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
#ifndef AGENT_CLIENT_AGENTCONN_H
#define AGENT_CLIENT_AGENTCONN_H

#include <base/mem/Buffer.h>
#include <base/mem/MemPool.h>
#include <event/EventLoop.h>
#include <proto/AgentProto.h>
#include <sock/Connector.h>

using namespace MSF::BASE;
using namespace MSF::SOCK;

namespace MSF {
namespace AGENT {

enum RecvState {
  kRecvBhs = 0x01,
  kRecvPdu = 0x02,
  kRecvNext = 0x03,
};

typedef std::unique_ptr<AgentProto> AgentProtoPtr;

class AgentConn;
typedef std::shared_ptr<AgentConn> AgentConnPtr;

class AgentConn : public Connector {
 private:
  EventCallback readCb_;
  EventCallback writeCb_;
  EventCallback errorCb_;
  EventCallback closeCb_;

  EventLoop *loop_;
  AgentProtoPtr proto_;
  MemPool *mpool_;

  Buffer readBuffer_;
  Buffer writeBuffer_;

  enum RecvState rxState_;

  uint32_t pduCount_;
  std::deque<struct iovec> rxQequeIov_;
  uint32_t rxNeedRecv_;
  uint32_t rxIovOffser_;

  std::mutex mutex_;
  std::vector<struct iovec> txQequeIov_;
  uint32_t txQequeSize_;

  std::vector<struct iovec> txBusyIov_;
  uint32_t txNeedSend_;  /* Current tx need send bytes */
  uint64_t txTotalSend_; /* Statitic bytes */
 private:
  void doRecvBhs();
  void doRecvPdu();

  void updateBusyIov();
  bool updateTxOffset(const int ret);

 public:
  void init(MemPool *mpool, EventLoop *loop, const int fd = -1) {
    mpool_ = mpool;
    loop_ = loop;
    fd_ = fd;
    Connector::init(loop, fd);
  }
  void wakeup() {
    // loop_->wakeup();
  }
  void doConnRead();
  void doConnWrite();
  void doConnClose();
  /* Nonelock per one thread */
  bool writeIovec(struct iovec iov);
  bool writeBuffer(char *data, const uint32_t len);
  bool writeBuffer(void *data, const uint32_t len);

  const uint32_t iovCount() const { return pduCount_; }

  struct iovec readIovec() {
    assert(pduCount_ > 0);
    --pduCount_;
    struct iovec iov(std::move(rxQequeIov_.front()));
    rxQequeIov_.pop_front();
    return iov;
  }

 public:
  AgentConn(/* args */);
  ~AgentConn();
};

}  // namespace AGENT
}  // namespace MSF
#endif
