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
#ifndef __MSF_CLI_AGENT_H__
#define __MSF_CLI_AGENT_H__

#include <sys/socket.h>
#include <unistd.h>

#include <functional>
#include <iostream>
#include <string>
// #include <sys/uio.h>
// #include <sys/socket.h>
#include <base/CountDownLatch.h>
#include <base/GccAttr.h>
#include <base/MemPool.h>
#include <event/EventLoop.h>
#include <proto/Protocol.h>
#include <sock/Connector.h>

using namespace MSF::BASE;
using namespace MSF::SOCK;
using namespace MSF::EVENT;
using namespace MSF::AGENT;

namespace MSF {
namespace AGENT {

typedef std::function<void(const char* data, const uint32_t len,
                           const uint32_t cmd)>
    AgentCb;

enum AgentType {
  kAgentUnix,
  kAgentTcp,
  kAgentUdp,
  kAgentMutiCast,
  kAgentNetLink,
};

#define MAX_CONN_NAME 32
#define MAX_CONN_IOV_SLOT 8
#define MAX_CONN_CMD_NUM 64

enum IOState {
  IO_INITIALIZED = 0,
  IO_READ_HALF = 1,
  IO_READ_DONE = 2,
  IO_WRITE_HALF = 3,
  IO_WRITE_DONE = 4,
  IO_CLOSED = 5,
};

enum RecvStage {
  STAGE_RECV_BHS = 0x01,
  STAGE_RECV_DATA = 0x02,
  STAGE_RECV_NEXT = 0x03,
};

enum SendStage {
  STAGE_SEND_BHS = 0x01,
  STAGE_SEND_DATA = 0x02,
  STAGE_SEND_NEXT = 0x03,
};

struct Chap {
  bool enable;
  uint32_t state;
  uint32_t alg;
  char user[MAX_CONN_NAME];
  char hash[32];
} MSF_PACKED_MEMORY;

#define ITEM_PER_ALLOC 4
#define ITEM_MAX_ALLOC 64

struct AgentCmd {
  struct AgentBhs bhs_;
  uint32_t state_;
  CountDownLatch latch_;
  // std::condition_variable ackSem_; /*semaphore used in queue*/

  AgentCb doneCb_; /*callback called in thread pool*/

  void* buffer_;
  uint32_t usedLen_;
  /* https://blog.csdn.net/ubuntu64fan/article/details/17629509 */
  /* ref_cnt aim to solve wildpointer and object, pointer retain */
  uint32_t refCnt_;
  bool needAck_;

  AgentCmd(const AgentCb& cb = AgentCb()) : latch_(1) {
    doneCb_ = std::move(cb);
  }
  ~AgentCmd() {}

  void doDecRef() { refCnt_++; }
  void doIncRef() { refCnt_--; }
  bool waitResponse(const std::mutex lock, const uint32_t timeout) {
    return true;
  }
};

class AgentClient {
 public:
  explicit AgentClient(const std::string& name, const enum AgentAppId cid,
                       const std::string& host = "127.0.0.1",
                       const uint16_t port = 8888);
  explicit AgentClient(const std::string& name, const enum AgentAppId cid,
                       const std::string& srvUnixPath,
                       const std::string& cliUnixPath);
  ~AgentClient();

  void setReqCb(const AgentCb& cb) { reqCb_ = std::move(cb); }
  bool init(EventLoop* loop);
  int sendPdu(const AgentPdu* pdu);

 private:
  bool started_;
  EventLoop* loop_;
  std::string srvIpAddr_;
  uint16_t srvPort_;
  std::string srvUnixPath_;
  std::string cliUnixPath_;
  std::string zkAddr_;
  uint16_t zkPort_;
  std::vector<std::tuple<std::string, uint16_t>> zkServer_;
  std::atomic_uint64_t msgSeq_; /*for client: ack seq num*/
  int eventFd_;

  std::string name_;
  enum AgentAppId cid_;

  AgentCb reqCb_;
  AgentCb ackCb_;

  AgentBhs bhs_;
  enum IOState ioState;
  enum SendStage _sendStage;
  enum RecvStage _recvStage;

  struct msghdr _rxHdr;
  ;
  struct msghdr _txHdr;
  struct iovec _rxIOV[MAX_CONN_IOV_SLOT];
  struct iovec _txIOV[MAX_CONN_IOV_SLOT];

  MemPool* mpool_;
  std::list<struct AgentCmd*> freeCmdList_;
  std::list<struct AgentCmd*> txCmdList_;  /* queue of tx cmd to handle*/
  std::list<struct AgentCmd*> txBusyList_; /* queue of tx cmd is handling */
  std::map<uint32_t, struct AgentCmd*>
      ackCmdMap_; /* queue of ack cmd to handle*/

  std::unique_ptr<Connector> conn_;

 private:
  struct AgentCmd* allocCmd(const uint32_t len);
  void freeCmd(struct AgentCmd* cmd);
  bool connectAgent();
  void connectAgentCb();
  void closeAgent();
  bool loginAgent();
  void handleTxCmd();
  void handleRxCmd();
  bool handleIORet(const int ret);
};

}  // namespace AGENT
}  // namespace MSF
#endif