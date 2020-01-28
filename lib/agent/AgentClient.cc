

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
#include <base/Logger.h>
#include <event/Event.h>
#include <event/EventLoop.h>

#include "AgentClient.h"

using namespace MSF::EVENT;

namespace MSF {
namespace AGENT {

#define DEFAULT_SRVAGENT_IPADDR "127.0.0.1"
#define DEFAULT_SRVAGENT_PORT   8888
#define DEFAULT_SRVAGENT_UNIXPATH "/vat/tmp/srv_agent.sock"
#define DEFAULT_CLIAGENT_UNIXPATH "/vat/tmp/cli_agent.sock"

AgentClient::AgentClient(const std::string & host, const uint16_t port)
    : started_(false), srvIpAddr_(host), srvPort_(port)
{
    name_ = "Mobile";
}

AgentClient::AgentClient(const std::string & srvUnixPath, const std::string & cliUnixPath)
    : started_(false), srvUnixPath_(srvUnixPath), cliUnixPath_(cliUnixPath)
{
}
AgentClient::~AgentClient()
{
    closeAgent();
    conn_ = nullptr;
    delete mpool_;
}

struct AgentCmd * AgentClient::allocCmd(const uint32_t len)
{
    void * buffer = mpool_->alloc(len);
    if (unlikely(buffer == nullptr)) {
        MSF_INFO << "Alloc cmd buffer failed";
        return nullptr;
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

    struct AgentCmd* cmd = freeCmdList_.front();
    cmd->buffer_ = buffer;
    freeCmdList_.pop_front();

    return cmd;
}

void AgentClient::freeCmd(struct AgentCmd * cmd)
{
    mpool_->free(cmd->buffer_);
    freeCmdList_.push_back(cmd);
}

bool AgentClient::connectAgent()
{
    if (!conn_) {
        conn_ = std::make_unique<Connector>();
        if (conn_ == nullptr) {
            MSF_ERROR << "Alloc event for agent faild";
            return false;
        }
    }


    bool success = false;
    if (!srvIpAddr_.empty()) {
        success = conn_->connect(srvIpAddr_.c_str(), srvPort_, SOCK_STREAM);
    } else if (!srvUnixPath_.empty() && !cliUnixPath_.empty()) {
        success = conn_->connect(srvUnixPath_.c_str(), cliUnixPath_.c_str());
    }

    if (!success) {
        return false;
    }

    conn_->init(loop_, conn_->fd());
    conn_->setSuccCallback(std::bind(&AgentClient::connectAgentCb, this));
    conn_->setReadCallback(std::bind(&AgentClient::handleRxCmd, this));
    conn_->setWriteCallback(std::bind(&AgentClient::handleTxCmd, this));
    conn_->setCloseCallback(std::bind(&AgentClient::closeAgent, this));
    conn_->setErrorCallback(std::bind(&AgentClient::closeAgent, this));
    conn_->enableEvent();

    if (!loginAgent()) {
        closeAgent();
        return false;
    }

    return true;
}

void AgentClient::connectAgentCb()
{

}

void AgentClient::closeAgent()
{
    conn_->close();
    started_ = false;
}

bool AgentClient::loginAgent()
{
    struct AgentBhs *bhs = nullptr;
    struct AgentLogin *login = nullptr;
    struct AgentCmd *req = nullptr;

    req = allocCmd(sizeof(struct AgentLogin));
    if (req == nullptr) {
        MSF_INFO << "Alloc login cmd buffer failed";
        return false;
    }

    bhs = &req->bhs_;
    login = (struct AgentLogin *)req->buffer_;
    bhs->magic      = RPC_MAGIC;
    bhs->version    = RPC_VERSION;
    bhs->opcode     = RPC_REQ;
    bhs->seq        = (++msgSeq_) % UINT64_MAX;
    bhs->timeout    = MSF_NONE_WAIT;
    bhs->cmd        = RPC_LOGIN;
    bhs->srcid      = cid_;
    bhs->dstid      = RPC_MSG_SRV_ID;
    bhs->datalen    = sizeof(struct AgentLogin);
    memset(login->name, 0, sizeof (login->name));
    memcpy(login->name, name_.c_str(), std::min(name_.size(), sizeof(login->name)));
    login->chap = false;
    req->usedLen_ = sizeof(struct AgentLogin);
    // req->doneCb_ = reqCb_;

    ackCmdMap_[bhs->seq] = req;
    txCmdList_.push_back(req);

    conn_->enableWriting();
    loop_->wakeup();

    return true;
}

bool AgentClient::handleIORet(const int ret)
{
    if (ret == 0) {
        closeAgent();
        txCmdList_.clear();
        return false;
    } else if (ret < 0 && 
            errno != EWOULDBLOCK &&
            errno != EAGAIN &&
            errno != EINTR) {
        closeAgent();
        txCmdList_.clear();
        return false;
    }
    return true;
}

void AgentClient::handleTxCmd()
{
    MSF_INFO << "Tx cmd : " << txCmdList_.size();
    if (txCmdList_.empty()) {
        return;
    }
    struct iovec iov[2];
    for (auto cmd : txCmdList_) {
        iov[0].iov_base = &cmd->bhs_;
        iov[0].iov_len = sizeof(struct AgentBhs);
        iov[1].iov_base = cmd->buffer_;
        iov[1].iov_len = cmd->usedLen_;

        if(!handleIORet(SendMsg(conn_->fd(), iov, 2, MSG_WAITALL | MSG_NOSIGNAL))) {
            return;
        };
    }
    txCmdList_.clear();
    conn_->disableWriting();
}

void AgentClient::handleRxCmd()
{
    struct iovec iov = { &bhs_, sizeof(AgentBhs)};
    if(!handleIORet(RecvMsg(conn_->fd(), &iov, 1, MSG_WAITALL | MSG_NOSIGNAL))) {
        return;
    };

    switch (bhs_.cmd) {
        case RPC_LOGIN:
            auto itor = ackCmdMap_.find(bhs_.seq);
            if (itor == ackCmdMap_.end()) {
                MSF_INFO << "Cannot find cmd in ack map";
                closeAgent();
            } else {
                if (bhs_.errcode == RPC_EXEC_SUCC) {
                    MSF_INFO << "Login to agent successful";
                    started_ = true;
                } else {
                    MSF_INFO << "Login to agent failed";
                    started_ = false;
                }

            }
            break;
    }
    // ackCmdMap_

}

bool AgentClient::init(EventLoop * loop)
{ 
    loop_ = loop;
    mpool_ = new MemPool();
    mpool_->init();
    connectAgent();
    return true;
}

int AgentClient::sendPdu(const AgentPdu & pdu)
{
    return 0;
}

// int CliAgent::producerRequest(struct basic_pdu *pdu)
// {
// #if 0
//     struct basic_head *bhs = NULL;
//     struct cmd *new_cmd = NULL;

    
//     bhs = &new_cmd->bhs;
//     bhs->magic   = RPC_MAGIC;
//     bhs->version = RPC_VERSION;
//     bhs->opcode  = RPC_REQ;
//     bhs->datalen = pdu->paylen;
//     bhs->restlen = pdu->restlen;

//     bhs->srcid =  cid;
//     bhs->dstid =  pdu->dstid;
//     bhs->cmd =  pdu->cmd;

//     msgSeq.fetch_add(1);
//     bhs->seq = msgSeq;
//     bhs->checksum = 0;
//     bhs->timeout  = pdu->timeout;

//     new_cmd->used_len = pdu->paylen; 
//     new_cmd->callback = req_scb;

//     if (pdu->paylen > 0) {
//         memcpy((char*)new_cmd->buffer, pdu->payload, pdu->paylen);
//     }


//     refcount_incr(&new_cmd->ref_cnt);
//     tx_cmd_list.push_back(req_cmd);


//     //notofy

//     //
    
//     if (MSF_NO_WAIT != pdu->timeout) {

//         msf_sem_init(&new_cmd->ack_sem);
//         /* Check what happened */
//         if (-1 == msf_sem_wait(&(new_cmd->ack_sem), pdu->timeout)) {
//             MSF_ERROR << "Wait for service ack timeout: " << pdu->timeout;
//             msf_sem_destroy(&(new_cmd)->ack_sem);
//             FreeCmd(new_cmd);
//             return -2;
//         } 

//         MSF_INFO << "Notify peer errcode ===>" << bhs->errcode;

//         if (likely(RPC_EXEC_SUCC == bhs->errcode)) {
//             memcpy(pdu->restload, (s8*)new_cmd->buffer, pdu->restlen);
//         }

//         msf_sem_destroy(&new_cmd->ack_sem);
//         FreeCmd(new_cmd);
//     }

//     return bhs->errcode;
// #endif
//     return 0;
// }


}
}