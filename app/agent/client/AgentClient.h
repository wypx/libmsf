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
#ifndef AGENT_CLIENT_AGENTCLIENT_H
#define AGENT_CLIENT_AGENTCLIENT_H

#include <sys/socket.h>
#include <unistd.h>

#include <functional>
#include <iostream>
#include <string>
#include <base/CountDownLatch.h>
#include <base/GccAttr.h>

#include <base/MemPool.h>
#include <event/EventLoop.h>
#include <proto/AgentProto.h>
#include <sock/Connector.h>

using namespace MSF::BASE;
using namespace MSF::SOCK;
using namespace MSF::EVENT;
using namespace MSF::AGENT;

namespace MSF {
namespace AGENT {

typedef std::function<void(char *data, uint32_t *len, const Agent::Command cmd)> AgentCb;

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
  uint32_t state_;
  CountDownLatch latch_;
  AgentCb doneCb_; /*callback called in thread pool*/

  Agent::AgentBhs bhs_;
  struct iovec iov[2];
  /* https://blog.csdn.net/ubuntu64fan/article/details/17629509 */
  /* ref_cnt aim to solve wildpointer and object, pointer retain */
  uint32_t refCnt_;
  Agent::Errno retCode_;
  uint32_t timeOut_;
  bool needAck_;

  void addHead(void *head, const uint32_t len)
  {
    iov[0].iov_base = head;
    iov[0].iov_len = len;
  }

  void addBody(void *body, const uint32_t len)
  {
    iov[1].iov_base = body;
    iov[1].iov_len = len;
  }

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
  explicit AgentClient(EventLoop *loop,
                      const std::string& name,
                      const Agent::AppId cid,
                      const std::string& host = "127.0.0.1",
                      const uint16_t port = 8888);
  explicit AgentClient(EventLoop *loop,
                      const std::string& name,
                      const Agent::AppId cid,
                      const std::string& srvUnixPath,
                      const std::string& cliUnixPath);
  ~AgentClient();

  void setReqCb(const AgentCb& cb) { reqCb_ = std::move(cb); }
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
  std::atomic_uint16_t msgSeq_; /*for client: ack seq num*/
  int eventFd_;

  std::string name_;
  Agent::AppId cid_;

  AgentCb reqCb_;
  AgentCb ackCb_;

  Agent::AgentBhs bhs_;
  enum IOState ioState;
  enum SendStage _sendStage;
  enum RecvStage _recvStage;

  struct msghdr _rxHdr;
  struct msghdr _txHdr;
  struct iovec _rxIOV[MAX_CONN_IOV_SLOT];
  struct iovec _txIOV[MAX_CONN_IOV_SLOT];


  MemPool* mpool_;
  std::list<struct AgentCmd*> freeCmdList_;
  std::list<struct AgentCmd*> txCmdList_;  /* queue of tx cmd to handle*/
  std::list<struct AgentCmd*> txBusyList_; /* queue of tx cmd is handling */
  std::map<uint32_t, struct AgentCmd*> ackCmdMap_;

  std::unique_ptr<AgentProto> proto_;
  ConnectionPtr conn_;
  bool reConnect_;  /* Enable reconnct agent server*/
  bool reConnecting_;
 private:
  struct AgentCmd* allocCmd(const uint32_t len);
  void freeCmd(struct AgentCmd* cmd);
  void connectAgent();
  void connectAgentCb();
  void closeAgent();
  bool loginAgent();
  void handleTxCmd();
  void handleRxCmd();
  void handleRequest(Agent::AgentBhs & bhs);
  void handleResponce(Agent::AgentBhs & bhs);
  bool handleIORet(const int ret);
};

}  // namespace AGENT
}  // namespace MSF
#endif