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

  loop_->runInLoop(std::bind(&AgentClient::connectAgent, this));
}

AgentClient::~AgentClient() {
  // conn_->reset();
  closeAgent();
  conn_ = nullptr;
  delete mpool_;
  mpool_ = nullptr;
}

struct AgentCmd *AgentClient::allocCmd(const uint32_t len) {
  if (unlikely(freeCmdList_.empty())) {
    for (uint32_t i = 0; i < ITEM_PER_ALLOC; ++i) {
      struct AgentCmd *tmp = new AgentCmd();
      if (tmp == nullptr) {
        return nullptr;
      }
      freeCmdList_.push_back(tmp);
    }
  }

  struct AgentCmd *cmd = freeCmdList_.front();
  cmd->latch_.setCount(1);
  freeCmdList_.pop_front();
  return cmd;
}

void AgentClient::freeCmd(struct AgentCmd *cmd) {
  if ((--cmd->refCnt_) < 0) {
    mpool_->free(cmd->iov[0].iov_base);
    mpool_->free(cmd->iov[1].iov_base);
    cmd->iov[0].iov_base = nullptr;
    cmd->iov[1].iov_base = nullptr;
    cmd->iov[0].iov_len = 0;
    cmd->iov[1].iov_len = 0;
    freeCmdList_.push_back(cmd);
  }
}

void AgentClient::connectAgent() {
  if (started_) {
    MSF_INFO << "Agent client has been connected.";
    return;
  }
  MSF_INFO << "Agent client is connecting.";

  if (!conn_) {
    conn_ = std::make_unique<Connector>();
    if (conn_ == nullptr) {
      MSF_ERROR << "Alloc conn for agent faild";
      return;
    }
  }

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

  if (!proto_) {
    proto_ = std::make_unique<AgentProto>(conn_, mpool_);
    if (proto_ == nullptr) {
      MSF_ERROR << "Alloc proto_ for agent faild";
      reConnecting_ = false;
      closeAgent();
      return;
    }
  }

  conn_->init(loop_, conn_->fd());
  conn_->setSuccCallback(std::bind(&AgentClient::connectAgentCb, this));
  conn_->setReadCallback(std::bind(&AgentClient::handleRxCmd, this));
  conn_->setWriteCallback(std::bind(&AgentClient::handleTxCmd, this));
  conn_->setCloseCallback(std::bind(&AgentClient::closeAgent, this));
  conn_->setErrorCallback(std::bind(&AgentClient::closeAgent, this));
  conn_->enableEvent();

  if (!loginAgent()) {
    reConnecting_ = false;
    closeAgent();
    return;
  }
  return;
}

void AgentClient::connectAgentCb() {}

void AgentClient::closeAgent() {
  conn_->close();
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

  MSF_INFO << "Login pb size: " << login.ByteSizeLong();

  struct AgentCmd *req = allocCmd(login.ByteSizeLong());
  if (req == nullptr) {
    MSF_INFO << "Alloc login cmd buffer failed";
    return false;
  }
  req->timeOut_ = MSF_NONE_WAIT;

  /* Default check param, not change usually */
  proto_->setVersion(bhs_, kAgentVersion);
  proto_->setEncrypt(bhs_, kAgentEncryptZip);
  proto_->setMagic(bhs_, kAgentMagic);

  uint16_t sessNo = (++msgSeq_) % UINT16_MAX;

  MSF_INFO << "sessNo: " << sessNo;
  MSF_INFO << "Bhs encrypt: " << std::hex << (uint32_t)proto_->encrypt(bhs_);

  // 不压缩也不加密
  // https://blog.csdn.net/u012707739/article/details/80084440?depth_1.utm_source=distribute.pc_relevant.none-task&utm_source=distribute.pc_relevant.none-task
  // https://blog.csdn.net/carbon06/article/details/80856436?utm_source=blogxgwz6
  // if ((uint32_t)proto_->encrypt(bhs_) == 0)
  {
    proto_->setSrcId(bhs_, cid_);
    proto_->setDstId(bhs_, Agent::AppId::APP_AGENT);
    proto_->setCommand(bhs_, Agent::Command::CMD_REQ_NODE_REGISTER);
    proto_->setOpcode(bhs_, Agent::Opcode::OP_REQ);
    proto_->setSessNo(bhs_, sessNo);
    proto_->setPduLen(bhs_, login.ByteSizeLong());

    proto_->debugBhs(bhs_);
    MSF_INFO << "Bhs pb size: " << bhs_.ByteSizeLong();

    void *head = mpool_->alloc(AGENT_HEAD_LEN);
    assert(head);
    bhs_.SerializeToArray(head, AGENT_HEAD_LEN);
    req->addHead(head, AGENT_HEAD_LEN);
    /* 无包体(心跳包等),在proto3的使用上以-1表示包体长度为0 */
    void *body = mpool_->alloc(login.ByteSizeLong());
    assert(body);
    login.SerializeToArray(body, login.ByteSizeLong());
    req->addBody(body, login.ByteSizeLong());
  
    req->doIncRef();
    ackCmdMap_[sessNo] = req;
    txCmdList_.push_back(req);

    conn_->enableWriting();
    loop_->wakeup();
  }
  return true;
}

bool AgentClient::handleIORet(const int ret) {
  if (ret == 0) {
    closeAgent();
    txCmdList_.clear();
    return false;
  } else if (ret < 0 && errno != EWOULDBLOCK && errno != EAGAIN &&
             errno != EINTR) {
    closeAgent();
    txCmdList_.clear();
    return false;
  }
  return true;
}

void AgentClient::handleTxCmd() {
  MSF_INFO << "Tx cmd : " << txCmdList_.size();
  if (txCmdList_.empty()) {
    return;
  }
  
  uint32_t iovCnt = 1;
  for (auto cmd : txCmdList_) {
    freeCmd(cmd);
    if (cmd->iov[1].iov_len) {
      iovCnt = 2;
    }
    int ret = SendMsg(conn_->fd(), cmd->iov, iovCnt, MSG_DONTWAIT | MSG_NOSIGNAL);
    MSF_INFO << "Send ret: " << cmd->iov[0].iov_len;
    MSF_INFO << "Send ret: " << cmd->iov[1].iov_len;
    MSF_INFO << "Send ret: " << ret;
    freeCmd(cmd);
  }
  txCmdList_.clear();
  conn_->disableWriting();
}

void AgentClient::handleRxCmd() {
  MSF_INFO << "Recv msg from fd: " << conn_->fd();
  void *head = mpool_->alloc(AGENT_HEAD_LEN);
  assert(head);
  struct iovec iov = {head, AGENT_HEAD_LEN};
  RecvMsg(conn_->fd(), &iov, 1, MSG_DONTWAIT | MSG_NOSIGNAL);
  Agent::AgentBhs bhs;
  bhs.ParseFromArray(head, AGENT_HEAD_LEN);
  proto_->debugBhs(bhs);

  mpool_->free(head);

  if (proto_->opCode(bhs) == Agent::Opcode::OP_REQ) {
    handleRequest(bhs);
  } else {
    handleResponce(bhs);
  }
}

void AgentClient::handleRequest(Agent::AgentBhs &bhs) {
  AgentCmd *cmd = allocCmd(proto_->pduLen(bhs));
  assert(cmd);
  if (proto_->pduLen(bhs)) {
    ///
  }

  {
    Agent::AppId tmpId = proto_->srcId(bhs);
    proto_->setSrcId(bhs, proto_->dstId(bhs));
    proto_->setDstId(bhs, tmpId);
  }
  proto_->setOpcode(bhs, Agent::Opcode::OP_RSP);

  uint32_t len = 0;

  void *body = mpool_->alloc(1024);
  if (unlikely(body == nullptr)) {
    MSF_INFO << "Alloc cmd buffer failed";
    return;
  }
  assert(body);

  if (reqCb_) {
    reqCb_(static_cast<char *>(body), &len,
           static_cast<Agent::Command>(proto_->command(bhs)));
  }
  MSF_INFO << "len: " << len;

  cmd->addBody(body, len);

  proto_->setPduLen(bhs, len);

  void *head = mpool_->alloc(AGENT_HEAD_LEN);
  assert(head);
  cmd->iov[0].iov_base = head;
  cmd->iov[0].iov_len = AGENT_HEAD_LEN;
  bhs.SerializeToArray(cmd->iov[0].iov_base, AGENT_HEAD_LEN);

  proto_->debugBhs(bhs);

  cmd->doIncRef();
  txCmdList_.push_back(cmd);

  conn_->enableWriting();
  loop_->wakeup();
}
void AgentClient::handleResponce(Agent::AgentBhs &bhs) {
  switch (static_cast<Agent::Command>(proto_->command(bhs_))) {
    case Agent::Command::CMD_REQ_NODE_REGISTER: {
      auto itor = ackCmdMap_.find(proto_->sessNo(bhs));
      if (itor == ackCmdMap_.end()) {
        MSF_INFO << "Cannot find cmd in ack map";
        //清空map
        closeAgent();
      } else {
        struct AgentCmd *req = (struct AgentCmd *)itor->second;
        if (proto_->retCode(bhs) == Agent::ERR_EXEC_SUCESS) {
          MSF_INFO << "Login to agent successful";
          started_ = true;
          reConnecting_ = false;
        } else {
          MSF_INFO << "Login to agent failed";
          started_ = false;
          reConnecting_ = false;
        }
        ackCmdMap_.erase(itor);
        freeCmd(req);
      }
    } break;
    default: {
      auto itor = ackCmdMap_.find(proto_->sessNo(bhs));
      if (itor != ackCmdMap_.end()) {
        struct AgentCmd *req = (struct AgentCmd *)itor->second;

        if (proto_->pduLen(bhs)) {
          void *body = mpool_->alloc(proto_->pduLen(bhs));
          struct iovec iov = {body, proto_->pduLen(bhs)};
          RecvMsg(conn_->fd(), &iov, 1, MSG_WAITALL | MSG_NOSIGNAL);
          req->addBody(body, proto_->pduLen(bhs));
        }

        if (proto_->retCode(bhs) == Agent::ERR_EXEC_SUCESS) {
          MSF_INFO << "Do cmd successful";
        } else {
          MSF_INFO << "Do cmd failed, retcode: " << proto_->retCode(bhs);
        }
        req->retCode_ = proto_->retCode(bhs);

        if (req->timeOut_) {
          // req->bhs_.retCode_ = bhs.retCode_;
          req->latch_.countDown();
          ackCmdMap_.erase(itor);
        } else {
          // reqCb_(static_cast<char*>(req->buffer_), bhs->dataLen_, bhs->cmd_);
        }

        freeCmd(req);
      } else {
        MSF_INFO << "Cannot find cmd for " << proto_->sessNo(bhs);
      }
    } break;
  }
}

int AgentClient::sendPdu(const AgentPdu *pdu) {
  struct AgentCmd *req = nullptr;

  if (!started_ || !loop_) {
    // MSF_ERROR << "Cannot send pdu, agent not start.";
    return Agent::ERR_AGENT_NOT_START;
  }

  /* Cannot in loop thread */
  if (loop_->isInLoopThread()) {
    MSF_ERROR << "Cannot send pdu in loop thread.";
    return Agent::ERR_CANNOT_IN_LOOP;
  }

  req = allocCmd(pdu->payLen_);
  if (req == nullptr) {
    MSF_INFO << "Alloc login cmd buffer failed";
    return false;
  }

  if (pdu->payLen_) {
    void *body = mpool_->alloc(pdu->payLen_);
    if (unlikely(body == nullptr)) {
      MSF_INFO << "Alloc cmd buffer failed";
      return -1;
    }
    if (pdu->payLen_) {
      memcpy(body, pdu->payLoad_, pdu->payLen_);
    }
    req->addBody(body, pdu->payLen_);
  }

  uint16_t sessNo = (++msgSeq_) % UINT16_MAX;
  proto_->setSrcId(bhs_, cid_);
  proto_->setDstId(bhs_, pdu->dstId_);
  proto_->setCommand(bhs_, pdu->cmd_);
  proto_->setOpcode(bhs_, Agent::Opcode::OP_REQ);
  proto_->setSessNo(bhs_, sessNo);
  proto_->setPduLen(bhs_, pdu->payLen_);

  proto_->debugBhs(bhs_);
  MSF_INFO << "Bhs pb size: " << bhs_.ByteSizeLong();

  void *head = mpool_->alloc(AGENT_HEAD_LEN);
  assert(head);
  bhs_.SerializeToArray(head, AGENT_HEAD_LEN);

  req->addHead(head, AGENT_HEAD_LEN);

  /* 无包体(心跳包等),在proto3的使用上以-1表示包体长度为0 */
  // void *body = mpool_->alloc(login.ByteSizeLong());
  // assert(body);
  // login.SerializeToArray(body, login.ByteSizeLong());
  // req->addBody(body, login.ByteSizeLong());

  MSF_ERROR << "Send pdu ======>.";
  req->doIncRef();
  txCmdList_.push_back(req);

  conn_->enableWriting();
  loop_->wakeup();

  if (pdu->timeOut_) {
    MSF_INFO << "=====================";
    req->timeOut_ = pdu->timeOut_;
    ackCmdMap_[sessNo] = req;
    req->doIncRef();
    /* Check what happened */
    if (!req->latch_.waitFor(pdu->timeOut_)) {
      MSF_ERROR << "Wait for service ack timeout: " << pdu->timeOut_;
      freeCmd(req);
      return Agent::ERR_RECV_TIMEROUT;
    };

    MSF_INFO << "Notify peer errcode ===>" << proto_->retCode(bhs_);
    MSF_INFO << "Notify peer errcode ===>" << req->retCode_;

    if (likely(Agent::ERR_EXEC_SUCESS == proto_->retCode(bhs_))) {
      // pdu->restLen_ req->usedLen
      memcpy(pdu->restLoad_, req->iov[1].iov_base, req->iov[0].iov_len);
    }
    freeCmd(req);
  }
  MSF_INFO << "==========*****===========";
  // return bhs->retCode_;
  return Agent::ERR_EXEC_SUCESS;
}

}  // namespace AGENT
}  // namespace MSF