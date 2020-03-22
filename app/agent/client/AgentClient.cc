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
#include "AgentClient.h"
#include <base/Logger.h>
#include <event/Event.h>
#include <proto/AgentProto.h>

#include <cassert>

using namespace MSF::EVENT;
using namespace MSF::AGENT;

namespace MSF {
namespace AGENT {

#define DEFAULT_SRVAGENT_IPADDR "127.0.0.1"
#define DEFAULT_SRVAGENT_PORT 8888

AgentClient::AgentClient() {
  mpool_ = new MemPool();
  assert(mpool_);
  assert(mpool_->init());

  proto_ = std::make_unique<AgentProto>();
  assert(proto_);

  conn_ = std::make_unique<AgentConn>();
  assert(conn_);

  loop_->runInLoop(std::bind(&AgentClient::connectAgent, this));
}

AgentClient::AgentClient(EventLoop *loop, const std::string &name,
                         const Agent::AppId cid, const std::string &host,
                         const uint16_t port)
    : started_(false),
      loop_(loop),
      srvIpAddr_(host),
      srvPort_(port),
      name_(name),
      cid_(cid),
      reqCb_(),
      reConnect_(true) {
  mpool_ = new MemPool();
  assert(mpool_);
  assert(mpool_->init());

  proto_ = std::make_unique<AgentProto>();
  assert(proto_);

  conn_ = std::make_unique<AgentConn>();
  assert(conn_);

  loop_->runInLoop(std::bind(&AgentClient::connectAgent, this));
}

AgentClient::AgentClient(EventLoop *loop, const std::string &name,
                         const Agent::AppId cid, const std::string &srvUnixPath,
                         const std::string &cliUnixPath)
    : started_(false),
      loop_(loop),
      srvUnixPath_(srvUnixPath),
      cliUnixPath_(cliUnixPath),
      name_(name),
      cid_(cid),
      reqCb_(),
      reConnect_(true) {
  mpool_ = new MemPool();
  assert(mpool_);
  assert(mpool_->init());

  proto_ = std::make_unique<AgentProto>();
  assert(proto_);

  conn_ = std::make_unique<AgentConn>();
  assert(conn_);

  loop_->runInLoop(std::bind(&AgentClient::connectAgent, this));
}

AgentClient::~AgentClient() {
  // conn_->reset();
  closeAgent();
  conn_ = nullptr;
  delete mpool_;
  mpool_ = nullptr;
}

char * AgentClient::allocBuffer(const uint32_t len) {
  return static_cast<char*>(mpool_->alloc(len));
}

void AgentClient::connectAgent() {
  if (started_) {
    MSF_INFO << "Agent client has been connected.";
    return;
  }
  MSF_INFO << "Agent client is connecting.";

  bool success = false;
  if (!srvIpAddr_.empty()) {
    success = conn_->connect(srvIpAddr_.c_str(), srvPort_, SOCK_STREAM);
  } else if (!srvUnixPath_.empty() && !cliUnixPath_.empty()) {
    success = conn_->connect(srvUnixPath_.c_str(), cliUnixPath_.c_str());
  }

  if (!success) {
    reConnecting_ = false;
    closeAgent();
    return;
  }

  conn_->init(mpool_, loop_, conn_->fd());
  conn_->setSuccCallback(std::bind(&AgentClient::connectAgentCb, this));
  conn_->setReadCallback(std::bind(&AgentClient::handleRxCmd, this));
  conn_->setWriteCallback(std::bind(&AgentClient::handleTxCmd, this));
  conn_->setCloseCallback(std::bind(&AgentClient::closeAgent, this));
  conn_->enableBaseEvent();

  if (!loginAgent()) {
    reConnecting_ = false;
    closeAgent();
    return;
  }
  return;
}

void AgentClient::connectAgentCb() {}

void AgentClient::closeAgent() {
  conn_->doConnClose();
  started_ = false;

  if (reConnect_ && !reConnecting_) {
    reConnecting_ = true;
    loop_->runAfter(2000, std::bind(&AgentClient::connectAgent, this));
  }
}

bool AgentClient::loginAgent() {
  Agent::LoginRequest login;
  login.set_name(name_);
  login.set_hash(0);
  login.set_chap(false);
  login.set_net(Agent::NET_ETH);
  login.set_token("token_" + name_);

  MSF_INFO << "Login pb size: " << login.ByteSizeLong();

  Agent::AgentBhs bhs;
  proto_->setVersion(bhs, kAgentVersion);
  proto_->setEncrypt(bhs, kAgentEncryptZip);
  proto_->setMagic(bhs, kAgentMagic);

  uint16_t sessNo = (++msgSeq_) % UINT16_MAX;

  MSF_INFO << "sessNo: " << sessNo;
  MSF_INFO << "Bhs encrypt: " << std::hex << (uint32_t)proto_->encrypt(bhs);

  // 不压缩也不加密
  // https://blog.csdn.net/u012707739/article/details/80084440?depth_1.utm_source=distribute.pc_relevant.none-task&utm_source=distribute.pc_relevant.none-task
  // https://blog.csdn.net/carbon06/article/details/80856436?utm_source=blogxgwz6
  // if ((uint32_t)proto_->encrypt(bhs_) == 0)
  {
    proto_->setSrcId(bhs, cid_);
    proto_->setDstId(bhs, Agent::AppId::APP_AGENT);
    proto_->setCommand(bhs, Agent::Command::CMD_REQ_NODE_REGISTER);
    proto_->setOpcode(bhs, Agent::Opcode::OP_REQ);
    proto_->setSessNo(bhs, sessNo);
    proto_->setPduLen(bhs, login.ByteSizeLong());

    proto_->debugBhs(bhs);
    MSF_INFO << "Bhs pb size: " << bhs.ByteSizeLong();

    void *head = mpool_->alloc(AGENT_HEAD_LEN);
    assert(head);
    bhs.SerializeToArray(head, AGENT_HEAD_LEN);

    conn_->writeBuffer(head, AGENT_HEAD_LEN);

    /* 无包体(心跳包等),在proto3的使用上以-1表示包体长度为0 */
    void *body = mpool_->alloc(login.ByteSizeLong());
    assert(body);
    login.SerializeToArray(body, login.ByteSizeLong());
    conn_->writeBuffer(body, login.ByteSizeLong());

    conn_->enableWriting();
    loop_->wakeup();
  }
  return true;
}

void AgentClient::handleTxCmd() {
  conn_->doConnWrite();
}

void AgentClient::handleRxCmd() {

  conn_->doConnRead();

  while (conn_->iovCount() > 0)
  {
    MSF_INFO << "======> handle iovec: " << conn_->iovCount();
    struct iovec head = conn_->readIovec();
    Agent::AgentBhs bhs;
    bhs.ParseFromArray(head.iov_base, AGENT_HEAD_LEN);
    proto_->debugBhs(bhs);
    if (proto_->opCode(bhs) == Agent::Opcode::OP_REQ) {
      handleRequest(bhs, head);
    } else {
      handleResponce(bhs, head);
    }
    mpool_->free(head.iov_base);
      // mpool_->free(pdu.second.iov_base);
  }
  
}

void AgentClient::handleRequest(Agent::AgentBhs &bhs, struct iovec & head) {

  if (proto_->pduLen(bhs)) {
    // struct iovec body = conn_->readIovec();
  }
  {
    Agent::AppId tmpId = proto_->srcId(bhs);
    proto_->setSrcId(bhs, proto_->dstId(bhs));
    proto_->setDstId(bhs, tmpId);
  }
  proto_->setOpcode(bhs, Agent::Opcode::OP_RSP);

  uint32_t len = 0;
  char *body = nullptr;
  if (reqCb_) {
    reqCb_(static_cast<char**>(&body), &len,
           static_cast<Agent::Command>(proto_->command(bhs)));
  }
  MSF_INFO << "len: " << len;

  proto_->setPduLen(bhs, len);
  bhs.SerializeToArray(head.iov_base, AGENT_HEAD_LEN);
  proto_->debugBhs(bhs);

  conn_->writeIovec(head);
  conn_->writeBuffer(body, len);

  conn_->enableWriting();
  loop_->wakeup();
}

void AgentClient::handleLoginRsp(const Agent::AgentBhs &bhs) {
  if (proto_->retCode(bhs) == Agent::ERR_EXEC_SUCESS) {
    MSF_INFO << "Login to agent successful";
    started_ = true;
    reConnecting_ = false;
  } else {
    MSF_INFO << "Login to agent failed";
    started_ = false;
    reConnecting_ = false;
  }
}

void AgentClient::handleBasicRsp(const Agent::AgentBhs &bhs, struct iovec & body) {
  auto itor = ackCmdMap_.find(proto_->sessNo(bhs));
  if (itor != ackCmdMap_.end()) {
    AgentCmdPtr cmd = static_cast<AgentCmdPtr>(itor->second);
    if (proto_->pduLen(bhs)) {
      cmd->fillRspData(body);
    }

    if (proto_->retCode(bhs) == Agent::ERR_EXEC_SUCESS) {
      MSF_INFO << "Do cmd successful";
    } else {
      MSF_ERROR << "Do cmd failed, retcode:" << proto_->retCode(bhs);
    }
    cmd->retCode_ = proto_->retCode(bhs);

    if (cmd->timeOut_) {
      cmd->postAck();
      ackCmdMap_.erase(itor);
    } else {
      // rspCb_(static_cast<char*>(req->buffer_), bhs->dataLen_, bhs->cmd_);
    }
  } else {
    MSF_INFO << "Cannot find cmd for " << proto_->sessNo(bhs);
  }
}

void AgentClient::handleResponce(Agent::AgentBhs &bhs, struct iovec & body) {
  switch (static_cast<Agent::Command>(proto_->command(bhs))) {
    case Agent::Command::CMD_REQ_NODE_REGISTER: 
      handleLoginRsp(bhs);
      break;
    default:
      handleBasicRsp(bhs, body);
      break;
  }
}

int AgentClient::sendPdu(const AgentPdu *pdu) {
  if (!started_ || !loop_) {
    // MSF_DEBUG << "Cannot send pdu, agent not start.";
    return Agent::ERR_AGENT_NOT_START;
  }

  /* Cannot in loop thread */
  if (loop_->isInLoopThread()) {
    MSF_ERROR << "Cannot send pdu in loop thread.";
    return Agent::ERR_CANNOT_IN_LOOP;
  }

  Agent::AgentBhs bhs;
  uint16_t sessNo = (++msgSeq_) % UINT16_MAX;
  proto_->setSrcId(bhs, cid_);
  proto_->setDstId(bhs, pdu->dstId_);
  proto_->setCommand(bhs, pdu->cmd_);
  proto_->setOpcode(bhs, Agent::Opcode::OP_REQ);
  proto_->setSessNo(bhs, sessNo);
  proto_->setPduLen(bhs, pdu->payLen_);

  proto_->debugBhs(bhs);
  MSF_INFO << "Bhs pb size: " << bhs.ByteSizeLong();

  void *head = mpool_->alloc(AGENT_HEAD_LEN);
  assert(head);
  bhs.SerializeToArray(head, AGENT_HEAD_LEN);
  conn_->writeBuffer(head, AGENT_HEAD_LEN);

  if (pdu->payLen_) {
    void *body = mpool_->alloc(pdu->payLen_);
    assert(body);
    if (pdu->payLen_) {
      memcpy(body, pdu->payLoad_, pdu->payLen_);
    }
    conn_->writeBuffer(body, pdu->payLen_);
  }
  conn_->enableWriting();
  loop_->wakeup();

  if (pdu->timeOut_) {
    AgentCmdPtr cmd = std::make_shared<struct AgentCmd>();
    assert(cmd);
    cmd->timeOut_ = pdu->timeOut_;
    ackCmdMap_[sessNo] = cmd;
    if (!cmd->waitAck(pdu->timeOut_)) {
      MSF_ERROR << "Wait for ack timeout:" << pdu->timeOut_;
      return Agent::ERR_RECV_TIMEROUT;
    };

    MSF_INFO << "Notify peer errcode ===>" << cmd->retCode_;
    if (Agent::ERR_EXEC_SUCESS == cmd->retCode_) {
      memcpy(pdu->restLoad_, cmd->buffer_.iov_base, cmd->buffer_.iov_len);
    }
    return cmd->retCode_;
  }
  return Agent::ERR_EXEC_SUCESS;
}

}  // namespace AGENT
}  // namespace MSF