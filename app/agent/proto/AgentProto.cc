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
#include <base/Bitops.h>
#include <base/Logger.h>
#include <sock/Socket.h>
// #include <sock/Connector.h>
#include "AgentProto.h"

using namespace MSF::AGENT;

namespace MSF {
namespace AGENT {

const uint16_t AgentProto::magic(const Agent::AgentBhs & bhs) const
{
  return (uint16_t)(bhs.verify() >> 48);
}

const uint8_t AgentProto::version(const Agent::AgentBhs & bhs) const
{
  return static_cast<uint8_t>(bhs.verify() >> 40);
}

const uint8_t AgentProto::encrypt(const Agent::AgentBhs & bhs) const
{
  return static_cast<uint8_t>(bhs.verify() >> 32);
}

const uint32_t AgentProto::checkSum(const Agent::AgentBhs & bhs) const
{
  return (uint32_t)(bhs.verify());
}

const Agent::AgentAppId AgentProto::srcId(const Agent::AgentBhs & bhs) const
{
  uint16_t sid = (uint16_t)(bhs.router() >> 48);
  return static_cast<Agent::AgentAppId>(sid);
}

const Agent::AgentAppId AgentProto::dstId(const Agent::AgentBhs & bhs) const
{
  uint16_t did = (uint16_t)(bhs.router() >> 32);
  return static_cast<Agent::AgentAppId>(did);
}

const uint16_t AgentProto::sessNo(const Agent::AgentBhs & bhs) const
{
  return (uint16_t)(bhs.router() >> 16);
}

const Agent::AgentErrno AgentProto::retCode(const Agent::AgentBhs & bhs) const
{
  uint16_t err = (uint16_t)(bhs.router());
  return static_cast<Agent::AgentErrno>(err);
}

const uint16_t AgentProto::command(const Agent::AgentBhs & bhs) const
{
  return (uint16_t)(bhs.command() >> 48);
}

const Agent::AgentOpcode AgentProto::opCode(const Agent::AgentBhs & bhs) const
{
  uint8_t op = (uint8_t)(bhs.command() >> 32);
  return static_cast<Agent::AgentOpcode>(op);
}
const uint32_t AgentProto::pduLen(const Agent::AgentBhs & bhs) const
{
  return (uint32_t)(bhs.command());
}

// ab00 0110 0000 0000
void AgentProto::setMagic(Agent::AgentBhs & bhs, const uint16_t magic) {
  uint64_t verify = bhs.verify();
  BIT_CLEAR(verify, (static_cast<uint64_t>(0xff)) << 48);
  BIT_SET(verify, (static_cast<uint64_t>(magic)) << 48);
  bhs.set_verify(verify);
}

void AgentProto::setVersion(Agent::AgentBhs & bhs, const uint8_t version) {
  uint64_t verify = bhs.verify();
  BIT_CLEAR(verify, (static_cast<uint64_t>(0xf)) << 40);
  BIT_SET(verify, (static_cast<uint64_t>(version)) << 40);
  bhs.set_verify(verify);
}

void AgentProto::setEncrypt(Agent::AgentBhs & bhs, const uint8_t encrypt) {
  uint64_t verify = bhs.verify();
  BIT_CLEAR(verify, (static_cast<uint64_t>(0xf)) << 32);
  BIT_SET(verify, static_cast<uint64_t>(encrypt) << 32);
  bhs.set_verify(verify);
}

void AgentProto::setCheckSum(Agent::AgentBhs & bhs, const uint32_t checkSum)
{
  uint64_t verify = bhs.verify();
  BIT_CLEAR(verify, (static_cast<uint64_t>(0xffff)) << 32);
  BIT_SET(verify, static_cast<uint64_t>(checkSum) << 16);
  bhs.set_verify(verify);
}

void AgentProto::setSrcId(Agent::AgentBhs & bhs, const Agent::AgentAppId srcId)
{
  uint64_t router = bhs.router();
  BIT_CLEAR(router, (static_cast<uint64_t>(0xff)) << 48);
  BIT_SET(router, static_cast<uint64_t>(srcId) << 48);
  bhs.set_router(router);
}
void AgentProto::setDstId(Agent::AgentBhs & bhs, const Agent::AgentAppId dstId)
{
  uint64_t router = bhs.router();
  BIT_CLEAR(router, (static_cast<uint64_t>(0xff)) << 32);
  BIT_SET(router, static_cast<uint64_t>(dstId) << 32);
  bhs.set_router(router);
}

void AgentProto::setSessNo(Agent::AgentBhs & bhs, const uint16_t sessNo)
{
  uint64_t router = bhs.router();
  BIT_CLEAR(router, (static_cast<uint64_t>(0xff)) << 16);
  BIT_SET(router, static_cast<uint64_t>(sessNo) << 16);
  bhs.set_router(router);
}

void AgentProto::setRetCode(Agent::AgentBhs & bhs, const Agent::AgentErrno retcode)
{
  uint64_t router = bhs.router();
  BIT_CLEAR(router, static_cast<uint64_t>(0xff));
  BIT_SET(router, static_cast<uint64_t>(retcode));
  bhs.set_router(router);
}

void AgentProto::setCommand(Agent::AgentBhs & bhs, const Agent::AgentCommand cmd)
{
  uint64_t command = bhs.command();
  BIT_CLEAR(command, (static_cast<uint64_t>(0xff)) << 48);
  BIT_SET(command, static_cast<uint64_t>(cmd) << 48);
  bhs.set_command(command);
}
void AgentProto::setOpcode(Agent::AgentBhs & bhs, const Agent::AgentOpcode op)
{
  uint64_t command = bhs.command();
  BIT_CLEAR(command, (static_cast<uint64_t>(0x1)) << 32);
  BIT_SET(command, static_cast<uint64_t>(op) << 32);
  bhs.set_command(command);
}

void AgentProto::setPduLen(Agent::AgentBhs & bhs, const uint32_t pduLen)
{
  uint64_t command = bhs.command();
  BIT_CLEAR(command, (static_cast<uint64_t>(0xffff)));
  BIT_SET(command, static_cast<uint64_t>(pduLen));
  bhs.set_command(command);
}

void AgentProto::debugBhs(const Agent::AgentBhs & bhs)
{
  MSF_INFO << "Bhs:";
  MSF_INFO << "magic: " << std::hex << magic(bhs);
  MSF_INFO << "version: " << std::hex << (uint32_t)version(bhs);
  MSF_INFO << "encrypt: " << std::hex << (uint32_t)encrypt(bhs);
  MSF_INFO << "checkSum: " << std::hex << checkSum(bhs);

  MSF_INFO << "srcid: " << std::hex << srcId(bhs);
  MSF_INFO << "dstid: " << std::hex  << dstId(bhs);
  MSF_INFO << "sessno: " << std::hex << sessNo(bhs);
  MSF_INFO << "opcode: " << std::hex << opCode(bhs);

  MSF_INFO << "command: " << std::hex << command(bhs);
  MSF_INFO << "opcode: " << std::hex  << opCode(bhs);
  MSF_INFO << "pdulen: " << std::hex << pduLen(bhs);

  MSF_INFO << "command: " << std::hex << bhs.verify();
  MSF_INFO << "opcode: " << std::hex  << bhs.router();
  MSF_INFO << "pdulen: " << std::hex << bhs.command();
}

int AgentProto::SendPdu(const Agent::AgentBhs & bhs, const iovec & iov_body)
{
    /* Max protibuf limit */
    // if (pdu.ByteSizeLong() > 1000000) {
    //     return -1;
    // }
    uint32_t iovCnt = 1;
    struct iovec iov[2];
    // 不压缩也不加密
    if (this->encrypt(bhs) == 0) {
      void *head = mpool_->alloc(AGENT_HEAD_LEN);
      assert(head);
      bhs.SerializeToArray(head, AGENT_HEAD_LEN);

      iov[0].iov_base = head;
      iov[0].iov_len = AGENT_HEAD_LEN;

      /* 无包体(心跳包等),在proto3的使用上以-1表示包体长度为0 */
      if (pduLen(bhs) <= 0) {
          SendMsg(conn_->fd(), iov, iovCnt, MSG_NOSIGNAL | MSG_WAITALL);
          return 0;
      }

      // void *body = mpool_->alloc(pdu.ByteSizeLong());
      // assert(body);
      // pdu.SerializeToArray(body, AGENT_HEAD_LEN);
      // iov[1].iov_base = body;
      // iov[1].iov_len = pdu.ByteSizeLong();
      // iovCnt++;
    } else {
      //zip
      //gzip
      //aes
      //rc5
    }
  
    SendMsg(conn_->fd(), iov, iovCnt, MSG_NOSIGNAL | MSG_WAITALL);
    return 0;
}


int AgentProto::RecvPdu(Agent::AgentBhs & bhs, AgentPdu & pdu)
{
    // RecvMsg();
    int len = 0;
    if (len >= AGENT_HEAD_LEN)
    {
        void *head = mpool_->alloc(AGENT_HEAD_LEN);
        assert(head);
        bool success = bhs.ParseFromArray(head, AGENT_HEAD_LEN);
        if (success) {
            if (pduLen(bhs) <= 0) {
                // pBuff->SkipBytes(AGENT_HEAD_LEN);
                return 0;
            }
            void *body = mpool_->alloc(1);
            if (len >= AGENT_HEAD_LEN + pduLen(bhs)) {
                // success = pdu.ParseFromArray(body, bhs.len());
                if (success)
                {
                    // pBuff->SkipBytes(AGENT_HEAD_LEN + bhs->len());
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                return -2;
            }
        }
        else
        {
            return -1;
        }
    }
    else
    {
        return -2;
    }
}

}
}