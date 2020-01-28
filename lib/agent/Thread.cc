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
#include <base/GccAttr.h>
#include <base/Logger.h>
#include <agent/Thread.h>

using namespace MSF::BASE;
using namespace MSF::AGENT;

namespace MSF {
namespace AGENT {

static inline void debugBhs(struct AgentBhs *bhs)
{
    MSF_DEBUG << "###################################";
    MSF_DEBUG << "bhs:";
    MSF_DEBUG << "bhs version: "    << bhs->version;
    MSF_DEBUG << "bhs magic:  "     << bhs->magic;
    MSF_DEBUG << "bhs srcid: "      << bhs->srcid;
    MSF_DEBUG << "bhs dstid: "      << bhs->dstid;
    MSF_DEBUG << "bhs opcode: "     << bhs->opcode;
    MSF_DEBUG << "bhs cmd: "        << bhs->cmd;
    MSF_DEBUG << "bhs seq: "        << bhs->seq;
    MSF_DEBUG << "bhs errcode: "    << bhs->errcode;
    MSF_DEBUG << "bhs datalen: "    << bhs->datalen;
    MSF_DEBUG << "bhs restlen: "    << bhs->restlen;
    MSF_DEBUG << "bhs checksum: "   << bhs->checksum;
    MSF_DEBUG << "bhs timeout: "    << bhs->timeout;
    MSF_DEBUG << "###################################";
}

void CliRxThread::readBhs()
{
    struct AgentBhs *bhs;
}
void CliRxThread::handleBhs()
{
    if (unlikely(RPC_VERSION != bhs.version || 
                RPC_MAGIC != bhs.magic)) {
        MSF_ERROR << "Notify login rsp code: " << bhs.errcode;
        // conn_free(c);
        return ;
    }

        /* If bhs no data, handle bhs, otherwise goto recv data*/
    if (0 == bhs.datalen) {
        if (RPC_REQ == bhs.opcode) {
            // MSF_AGENT_LOG(DBG_INFO, "Direct handle req.");
            // handleReq();
        } else if (RPC_ACK == bhs.opcode) {
            // MSF_AGENT_LOG(DBG_INFO, "Direct handle ack.");
            // handleAckRsp();
        }
    } else {

    }
}
void CliRxThread::handleRet()
{

}
void CliRxThread::handleReq(struct cmd *c)
{
    struct AgentBhs *bhs = nullptr;
    // bhs = &c->bhs;

    // if (rpc->req_scb) {
    //     rpc->req_scb(new_cmd->buffer, bhs->datalen, bhs->cmd);
    // }
}
void CliRxThread::handleRsp(struct cmd *c)
{

}
void CliRxThread::handleAckData(struct cmd *c)
{

}
void CliRxThread::handleAckRsp(struct cmd *c)
{

}

}
}