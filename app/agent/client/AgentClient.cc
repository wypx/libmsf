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
#include <cassert>
#include <base/logging.h>
#include "AgentClient.h"
#include "Event.h"
#include "AgentProto.h"

using namespace MSF;

namespace MSF {

// https://blog.csdn.net/i_o_fly/article/details/80636146
void AgentClient::init(EventLoop *loop, const std::string &name,
                       const Agent::AppId cid) {
  started_ = false;
  loop_ = loop;
  name_ = name;
  cid_ = cid;
  reConnect_ = true;

  mpool_ = new MemPool();
  assert(mpool_);
  assert(mpool_->Init());

  proto_ = std::make_unique<AgentProto>();
  assert(proto_);

  conn_ = std::make_unique<AgentConn>();
  assert(conn_);

  loop_->runInLoop(std::bind(&AgentClient::connectAgent, this));
}

AgentClient::AgentClient(EventLoop *loop, const std::string &name,
                         const Agent::AppId cid, const std::string &host,
                         const uint16_t port)
    : srvIpAddr_(host), srvPort_(port) {
  init(loop, name, cid);
}

AgentClient::AgentClient(EventLoop *loop, const std::string &name,
                         const Agent::AppId cid, const std::string &srvUnixPath,
                         const std::string &cliUnixPath)
    : srvUnixPath_(srvUnixPath), cliUnixPath_(cliUnixPath) {
  init(loop, name, cid);
}

AgentClient::~AgentClient() {
  // conn_->reset();
  closeAgent();
  conn_ = nullptr;
  delete mpool_;
  mpool_ = nullptr;
}

char *AgentClient::allocBuffer(const uint32_t len) {
  return static_cast<char *>(mpool_->Alloc(len));
}

void AgentClient::connectAgent() {
  if (started_) {
    LOG(INFO) << "Agent client has been connected.";
    return;
  }
  LOG(INFO) << "Agent client is connecting.";

  bool success = false;
  if (!srvIpAddr_.empty()) {
    success = conn_->Connect(srvIpAddr_.c_str(), srvPort_, SOCK_STREAM);
  } else if (!srvUnixPath_.empty() && !cliUnixPath_.empty()) {
    success = conn_->Connect(srvUnixPath_.c_str(), cliUnixPath_.c_str());
  }

  if (!success) {
    reConnecting_ = false;
    closeAgent();
    return;
  }

  conn_->Init(loop_, conn_->fd());
  conn_->SetSuccCallback(std::bind(&AgentClient::connectAgentCb, this));
  conn_->SetReadCallback(std::bind(&AgentClient::handleRxCmd, this));
  conn_->SetWriteCallback(std::bind(&AgentClient::handleTxCmd, this));
  conn_->SetCloseCallback(std::bind(&AgentClient::closeAgent, this));
  conn_->EnableBaseEvent();

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

bool AgentClient::verifyAgentBhs(const Agent::AgentBhs &bhs) {
  if (unlikely(proto_->magic(bhs) != kAgentMagic)) {
    LOG(ERROR) << "Unkown agent magic from: " << proto_->srcId(bhs);
    return false;
  }
  if (unlikely(proto_->version(bhs) != kAgentVersion)) {
    LOG(ERROR) << "Unkown agent version from: " << proto_->srcId(bhs);
    return false;
  }
  return true;
}

bool AgentClient::loginAgent() {
  Agent::LoginRequest login;
  login.set_name(name_);

  Agent::Chap *chap = login.mutable_chap();
  chap->set_hash("abcd");
  chap->set_alg(1);
  chap->set_phase(2);
  login.set_net(Agent::NET_ETH);
  login.set_token("token_" + name_);

  LOG(INFO) << "Login pb size: " << login.ByteSizeLong();

  Agent::AgentBhs bhs;
  proto_->setVersion(bhs, kAgentVersion);
  proto_->setEncrypt(bhs, kAgentEncryptZip);
  proto_->setMagic(bhs, kAgentMagic);

  uint16_t sessNo = (++msgSeq_) % UINT16_MAX;

  LOG(INFO) << "sessNo: " << sessNo;
  LOG(INFO) << "Bhs encrypt: " << std::hex << (uint32_t)proto_->encrypt(bhs);

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
    LOG(INFO) << "Bhs pb size: " << bhs.ByteSizeLong();

    void *head = mpool_->Alloc(AGENT_HEAD_LEN);
    assert(head);
    bhs.SerializeToArray(head, AGENT_HEAD_LEN);

    conn_->writeBuffer(head, AGENT_HEAD_LEN);

    /* 无包体(心跳包等),在proto3的使用上以-1表示包体长度为0 */
    void *body = mpool_->Alloc(login.ByteSizeLong());
    assert(body);
    login.SerializeToArray(body, login.ByteSizeLong());
    conn_->writeBuffer(body, login.ByteSizeLong());

    conn_->EnableWriting();
    loop_->wakeup();
  }
  return true;
}

void AgentClient::handleTxCmd() { conn_->doConnWrite(); }

void AgentClient::handleRxCmd() {
  if (!conn_->doConnRead()) {
    closeAgent();
    return;
  }

  while (conn_->iovCount() > 0) {
    LOG(INFO) << "======> handle iovec: " << conn_->iovCount();
    struct iovec head = conn_->readIovec();
    Agent::AgentBhs bhs;
    bhs.ParseFromArray(head.iov_base, AGENT_HEAD_LEN);
    proto_->debugBhs(bhs);

    if (!verifyAgentBhs(bhs)) {
      closeAgent();
      return;
    }
    if (proto_->opCode(bhs) == Agent::Opcode::OP_REQ) {
      handleRequest(bhs, head);
    } else {
      handleResponce(bhs);
      mpool_->Free(head.iov_base);
    }
  }
}

void AgentClient::handleRequest(Agent::AgentBhs &bhs, struct iovec &head) {
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
    reqCb_(static_cast<char **>(&body), &len,
           static_cast<Agent::Command>(proto_->command(bhs)));
  }
  LOG(INFO) << "len: " << len;

  proto_->setPduLen(bhs, len);
  bhs.SerializeToArray(head.iov_base, AGENT_HEAD_LEN);
  proto_->debugBhs(bhs);

  conn_->writeIovec(head);
  conn_->writeBuffer(body, len);

  conn_->EnableWriting();
}

void AgentClient::handleLoginRsp(const Agent::AgentBhs &bhs) {
  if (proto_->retCode(bhs) == Agent::ERR_EXEC_SUCESS) {
    LOG(INFO) << "Login to agent successful";
    started_ = true;
    reConnecting_ = false;
  } else {
    LOG(INFO) << "Login to agent failed";
    started_ = false;
    reConnecting_ = false;
  }
}

void AgentClient::handleBasicRsp(const Agent::AgentBhs &bhs) {
  auto itor = ackCmdMap_.find(proto_->sessNo(bhs));
  if (itor != ackCmdMap_.end()) {
    AgentPduPtr pdu = static_cast<AgentPduPtr>(itor->second);
    pdu->retCode_ = proto_->retCode(bhs);
    if (proto_->pduLen(bhs)) {
      struct iovec body = conn_->readIovec();
      if (pdu->retCode_ == Agent::ERR_EXEC_SUCESS) {
        assert(body.iov_len == pdu->rspload_.iov_len);
        memcpy(pdu->rspload_.iov_base, body.iov_base, body.iov_len);
      }
      mpool_->Free(body.iov_base);
    }
    if (pdu->timeOut_) {
      pdu->postAck();
      ackCmdMap_.erase(itor);
    } else {
      // rspCb_(static_cast<char*>(req->buffer_), bhs->dataLen_, bhs->cmd_);
    }
  } else {
    LOG(INFO) << "Cannot find cmd for " << proto_->sessNo(bhs);
  }
}

void AgentClient::handleResponce(Agent::AgentBhs &bhs) {
  switch (static_cast<Agent::Command>(proto_->command(bhs))) {
    case Agent::Command::CMD_REQ_NODE_REGISTER:
      handleLoginRsp(bhs);
      break;
    default:
      handleBasicRsp(bhs);
      break;
  }
}

int AgentClient::sendPdu(const AgentPduPtr pdu) {
  if (!started_ || !loop_) {
    // LOG(DEBUG) << "Cannot send pdu, agent not start.";
    return Agent::ERR_AGENT_NOT_START;
  }

  /* Cannot in loop thread */
  if (loop_->isInLoopThread()) {
    LOG(ERROR) << "Cannot send pdu in loop thread.";
    return Agent::ERR_CANNOT_IN_LOOP;
  }

  Agent::AgentBhs bhs;
  uint16_t sessNo = (++msgSeq_) % UINT16_MAX;
  proto_->setVersion(bhs, kAgentVersion);
  proto_->setEncrypt(bhs, kAgentEncryptZip);
  proto_->setMagic(bhs, kAgentMagic);
  proto_->setSrcId(bhs, cid_);
  proto_->setDstId(bhs, pdu->dstId_);
  proto_->setCommand(bhs, pdu->cmd_);
  proto_->setOpcode(bhs, Agent::Opcode::OP_REQ);
  proto_->setSessNo(bhs, sessNo);
  proto_->setPduLen(bhs, pdu->payload_.iov_len);

  proto_->debugBhs(bhs);
  LOG(INFO) << "Bhs pb size: " << bhs.ByteSizeLong();

  void *head = mpool_->Alloc(AGENT_HEAD_LEN);
  assert(head);
  bhs.SerializeToArray(head, AGENT_HEAD_LEN);
  conn_->writeBuffer(head, AGENT_HEAD_LEN);

  if (pdu->payload_.iov_len) {
    void *body = mpool_->Alloc(pdu->payload_.iov_len);
    assert(body);
    memcpy(body, pdu->payload_.iov_base, pdu->payload_.iov_len);
    conn_->writeBuffer(body, pdu->payload_.iov_len);
  }
  conn_->EnableWriting();

  if (pdu->timeOut_) {
    ackCmdMap_[sessNo] = pdu;
    if (!pdu->waitAck(pdu->timeOut_)) {
      LOG(ERROR) << "Wait for ack timeout:" << pdu->timeOut_;
      return Agent::ERR_RECV_TIMEROUT;
    };
    LOG(INFO) << "Notify peer errcode ===>" << pdu->retCode_;
    return pdu->retCode_;
  }
  return Agent::ERR_EXEC_SUCESS;
}

}  // namespace MSF