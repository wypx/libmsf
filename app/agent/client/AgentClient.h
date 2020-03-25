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

#include <base/CountDownLatch.h>
#include <base/GccAttr.h>
#include <event/EventLoop.h>
#include <proto/AgentProto.h>
#include <client/AgentConn.h>

using namespace MSF::BASE;
using namespace MSF::SOCK;
using namespace MSF::EVENT;
using namespace MSF::AGENT;

namespace MSF {
namespace AGENT {

typedef std::function<void(char** data, uint32_t* len, const Agent::Command cmd)>
    AgentCb;

enum AgentType {
  kAgentUnix,
  kAgentTcp,
  kAgentUdp,
  kAgentMutiCast,
  kAgentNetLink,
};

#define ITEM_PER_ALLOC 4
#define ITEM_MAX_ALLOC 64

struct AgentPdu : public std::enable_shared_from_this<AgentPdu> {
  Agent::AppId dstId_;
  Agent::Command cmd_;
  Agent::Errno retCode_;
  uint32_t timeOut_;
  CountDownLatch latch_;
  AgentCb doneCb_; /*callback called in thread pool*/

  struct iovec payload_;
  struct iovec rspload_;

  AgentPdu(const AgentCb& cb = AgentCb()) : latch_(1) {
    memset(&payload_, 0, sizeof(struct iovec));
    memset(&rspload_, 0, sizeof(struct iovec));
    doneCb_ = std::move(cb);
  }
  ~AgentPdu() {}

  void addPayload(char *buffer, const uint32_t len) {
    payload_.iov_base = buffer;
    payload_.iov_len = len;
  }
  void addRspload(void *buffer, const uint32_t len) {
    addRspload(static_cast<char*>(buffer), len);
  }
  void addRspload(char *buffer, const uint32_t len) {
    rspload_.iov_base = buffer;
    rspload_.iov_len = len;
  }
  bool waitAck(const uint32_t ts) {
    return latch_.waitFor(ts);
  }
  void postAck() {
    return latch_.countDown();
  }
};

typedef std::shared_ptr<AgentPdu> AgentPduPtr;

class AgentClient {
 public:
  explicit AgentClient(EventLoop* loop, const std::string& name,
                       const Agent::AppId cid,
                       const std::string& host = "127.0.0.1",
                       const uint16_t port = 8888);
  explicit AgentClient(EventLoop* loop, const std::string& name,
                       const Agent::AppId cid, const std::string& srvUnixPath,
                       const std::string& cliUnixPath);
  ~AgentClient();

  void setRequestCb(const AgentCb& cb) { reqCb_ = std::move(cb); }
  int sendPdu(const AgentPduPtr pdu);
  char *allocBuffer(const uint32_t len);

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

  std::string name_;
  Agent::AppId cid_;

  AgentCb reqCb_;
  AgentCb rspCb_;

  MemPool* mpool_;
  std::map<uint32_t, AgentPduPtr> ackCmdMap_;

  AgentProtoPtr proto_;
  AgentConnPtr conn_;
  bool reConnect_; /* Enable reconnct agent server*/
  bool reConnecting_;

 private:
  void init(EventLoop *loop, const std::string &name,
                       const Agent::AppId cid);
  void connectAgent();
  void connectAgentCb();
  void closeAgent();
  bool loginAgent();
  void handleTxCmd();
  void handleRxCmd();
  bool verifyAgentBhs(const Agent::AgentBhs &bhs);
  void handleRequest(Agent::AgentBhs& bhs, struct iovec & head);
  void handleResponce(Agent::AgentBhs& bhs);
  void handleLoginRsp(const Agent::AgentBhs &bhs);
  void handleBasicRsp(const Agent::AgentBhs &bhs);
};

}  // namespace AGENT
}  // namespace MSF
#endif