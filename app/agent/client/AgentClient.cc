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

#include <cassert>

using namespace MSF::EVENT;

namespace MSF {
namespace AGENT {

#define DEFAULT_SRVAGENT_IPADDR "127.0.0.1"
#define DEFAULT_SRVAGENT_PORT 8888

inline void AgentClient::debugAgentBhs(struct AgentBhs *bhs) {
  MSF_DEBUG << "###################################";
  MSF_DEBUG << "bhs:";
  MSF_DEBUG << "bhs version: " << bhs->version_;
  MSF_DEBUG << "bhs magic:  " << bhs->magic_;
  MSF_DEBUG << "bhs srcid: " << bhs->srcId_;
  MSF_DEBUG << "bhs dstid: " << bhs->dstId_;
  MSF_DEBUG << "bhs cmd: " << bhs->cmd_;
  MSF_DEBUG << "bhs seq: " << bhs->sessNo_;
  MSF_DEBUG << "bhs errcode: " << bhs->retCode_;
  MSF_DEBUG << "bhs datalen: " << bhs->dataLen_;
  MSF_DEBUG << "bhs restlen: " << bhs->restLen_;
  MSF_DEBUG << "bhs checksum: " << bhs->checkSum_;
  MSF_DEBUG << "bhs timeout: " << bhs->timeOut_;
  MSF_DEBUG << "###################################";
}

AgentClient::AgentClient(EventLoop *loop,
                        const std::string & name,
                        const enum AgentAppId cid,
                        const std::string & host, const uint16_t port)
    : started_(false),
      loop_(loop),
      srvIpAddr_(host),
      srvPort_(port),
      name_(name),
      cid_(cid),
      reqCb_(),
      reConnect_(true)
{
  mpool_ = new MemPool();
  assert(mpool_);
  assert(mpool_->init());

  loop_->runInLoop(std::bind(&AgentClient::connectAgent, this));
}

AgentClient::AgentClient(EventLoop *loop,
                        const std::string & name,
                        const enum AgentAppId cid,
                        const std::string & srvUnixPath,
                        const std::string & cliUnixPath)
    : started_(false),
      loop_(loop),
      srvUnixPath_(srvUnixPath),
      cliUnixPath_(cliUnixPath),
      name_(name),
      cid_(cid),
      reqCb_(),
      reConnect_(true)
{
  mpool_ = new MemPool();
  assert(mpool_);
  assert(mpool_->init());

  loop_->runInLoop(std::bind(&AgentClient::connectAgent, this));
}

AgentClient::~AgentClient() {
  closeAgent();
  conn_ = nullptr;
  delete mpool_;
}

struct AgentCmd *AgentClient::allocCmd(const uint32_t len) {
  void *buffer = nullptr;
  if (len) {
    buffer = mpool_->alloc(len);
    if (unlikely(buffer == nullptr)) {
      MSF_INFO << "Alloc cmd buffer failed";
      return nullptr;
    }
  }

  if (unlikely(freeCmdList_.empty())) {
    for (uint32_t i = 0; i < ITEM_PER_ALLOC; ++i) {
      struct AgentCmd *tmp = new AgentCmd();
      if (tmp == nullptr) {
        mpool_->free(buffer);
        return nullptr;
      }
      freeCmdList_.push_back(tmp);
    }
  }

  struct AgentCmd *cmd = freeCmdList_.front();
  cmd->buffer_ = buffer;
  cmd->latch_.setCount(1);
  freeCmdList_.pop_front();

  return cmd;
}

void AgentClient::freeCmd(struct AgentCmd *cmd) {
  if ((--cmd->refCnt_) == 0) {
    mpool_->free(cmd->buffer_);
    freeCmdList_.push_back(cmd);
  }
}

void AgentClient::connectAgent() {

  MSF_INFO << "Agent client is connecting.";

  if (started_) {
    MSF_INFO << "Agent client has been connected.";
    return;
  }

  if (!conn_) {
    conn_ = std::make_unique<Connector>();
    if (conn_ == nullptr) {
      MSF_ERROR << "Alloc event for agent faild";
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

  reConnectFail_ = 0;

  return;
}

void AgentClient::connectAgentCb() {}

void AgentClient::closeAgent() {

    conn_->close();
    started_ = false;

    if (reConnect_ && !reConnecting_) {
      reConnecting_ = true;
      MSF_WARN << "Agent client reconnecting server.";
      loop_->runAfter(2000, std::bind(&AgentClient::connectAgent, this));
    }
}

bool AgentClient::loginAgent() {
  struct AgentBhs *bhs = nullptr;
  struct AgentLogin *login = nullptr;
  struct AgentCmd *req = nullptr;

  req = allocCmd(sizeof(struct AgentLogin));
  if (req == nullptr) {
    MSF_INFO << "Alloc login cmd buffer failed";
    return false;
  }

  bhs = &req->bhs_;
  memset(bhs, 0, sizeof(AgentBhs));
  login = (struct AgentLogin *)req->buffer_;
  bhs->magic_ = AGENT_MAGIC;
  bhs->version_ = AGENT_VERSION;
  bhs->sessNo_ = (++msgSeq_) % UINT64_MAX;
  bhs->timeOut_ = MSF_NONE_WAIT;
  bhs->cmd_ = AGENT_LOGIN_REQUEST;
  bhs->srcId_ = cid_;
  bhs->dstId_ = APP_AGENT;
  bhs->dataLen_ = sizeof(struct AgentLogin);
  memset(login->name_, 0, sizeof(login->name_));
  memcpy(login->name_, name_.c_str(),
         std::min(name_.size(), sizeof(login->name_)));
  login->chap_ = false;
  req->usedLen_ = sizeof(struct AgentLogin);
  // req->doneCb_ = reqCb_;

  debugAgentBhs(bhs);

  ackCmdMap_[bhs->sessNo_] = req;
  txCmdList_.push_back(req);

  conn_->enableWriting();
  loop_->wakeup();

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
  struct iovec iov[2];
  uint32_t iovCnt = 1;
  for (auto cmd : txCmdList_) {
    iov[0].iov_base = &cmd->bhs_;
    iov[0].iov_len = sizeof(struct AgentBhs);

    if (cmd->usedLen_) {
      iov[1].iov_base = cmd->buffer_;
      iov[1].iov_len = cmd->usedLen_;
      iovCnt++;
    }
    if (!handleIORet(
            SendMsg(conn_->fd(), iov, iovCnt, MSG_WAITALL | MSG_NOSIGNAL))) {
      return;
    };
  }
  txCmdList_.clear();
  conn_->disableWriting();
}

void AgentClient::handleRxCmd() {
  struct iovec iov = {&bhs_, sizeof(AgentBhs)};
  if (!handleIORet(RecvMsg(conn_->fd(), &iov, 1, MSG_WAITALL | MSG_NOSIGNAL))) {
    return;
  };
  debugAgentBhs(&bhs_);

  if (bhs_.opCode_ == AGETN_REQUEST) {
    handleRequest(&bhs_);
  } else {
    handleResponce(&bhs_);
  }

  // ackCmdMap_
}

void AgentClient::handleRequest(AgentBhs *bhs) {
    if (reqCb_) {
        reqCb_(nullptr, 0, bhs->cmd_);
    }
}
void AgentClient::handleResponce(AgentBhs *bhs)
{
  switch (bhs->cmd_) {
      case AGENT_LOGIN_REQUEST: {
        auto itor = ackCmdMap_.find(bhs_.sessNo_);
        if (itor == ackCmdMap_.end()) {
          MSF_INFO << "Cannot find cmd in ack map";
          //清空map
          closeAgent();
        } else {
          struct AgentCmd *req = (struct AgentCmd *)itor->second;
          if (bhs->retCode_ == AGENT_E_EXEC_SUCESS) {
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
      default:
        if (bhs->timeOut_) {
        auto itor = ackCmdMap_.find(bhs->sessNo_);
        if (itor != ackCmdMap_.end()) {
            if (bhs->retCode_ == AGENT_E_EXEC_SUCESS) {
              MSF_INFO << "Do cmd successful";
            } else {
              MSF_INFO << "Do cmd failed";
            }
            struct AgentCmd *req = (struct AgentCmd *)itor->second;
            req->bhs_.retCode_ = bhs->retCode_;
            req->latch_.countDown();
            ackCmdMap_.erase(itor);
            freeCmd(req);
        } else {
            MSF_INFO << "Cannot find cmd for " <<  bhs->sessNo_;
        }
    } break;
  }
}

int AgentClient::sendPdu(const AgentPdu *pdu) {
  struct AgentBhs *bhs = nullptr;
  struct AgentCmd *req = nullptr;

  if (!started_ || !loop_) {
    MSF_ERROR << "Cannot send pdu, agent not start.";
    return AGENT_E_AGENT_NOT_START;
  }

  /* Cannot in loop thread */
  if (loop_->isInLoopThread()) {
    MSF_ERROR << "Cannot send pdu in loop thread.";
    return AGENT_E_CANNOT_IN_LOOP;
  }

  req = allocCmd(pdu->payLen_);
  if (req == nullptr) {
    MSF_INFO << "Alloc login cmd buffer failed";
    return false;
  }

  if (pdu->payLen_) {
    memcpy(req->buffer_, pdu->payLoad_, pdu->payLen_);
  }
  bhs = &req->bhs_;
  memset(bhs, 0, sizeof(AgentBhs));
  bhs->magic_ = AGENT_MAGIC;
  bhs->version_ = AGENT_VERSION;
  bhs->sessNo_ = (++msgSeq_) % UINT64_MAX;
  bhs->timeOut_ = pdu->timeOut_;
  bhs->cmd_ = pdu->cmd_;
  bhs->srcId_ = cid_;
  bhs->dstId_ = pdu->dstId_;
  bhs->dataLen_ = pdu->payLen_;
  bhs->restLen_ = pdu->restLen_;
  bhs->opCode_ = AGETN_REQUEST;

  MSF_ERROR << "Send pdu ======>.";
  req->doIncRef();
  txCmdList_.push_back(req);

  conn_->enableWriting();
  loop_->wakeup();

  if (pdu->timeOut_) {

    MSF_INFO << "=====================";
    ackCmdMap_[bhs->sessNo_] = req;
    req->doIncRef();
    /* Check what happened */
    if (!req->latch_.waitFor(pdu->timeOut_)) {
      MSF_ERROR << "Wait for service ack timeout: " << pdu->timeOut_;
      freeCmd(req);
      return AGENT_E_RECV_TIMEROUT;
    };

    MSF_INFO << "Notify peer errcode ===>" << bhs->retCode_;

    if (likely(AGENT_E_EXEC_SUCESS == bhs->retCode_)) {
      memcpy(pdu->restLoad_, req->buffer_, pdu->restLen_);
    }
    freeCmd(req);
  }
  MSF_INFO << "==========*****===========";
  return bhs->retCode_;
}

}  // namespace AGENT
}  // namespace MSF