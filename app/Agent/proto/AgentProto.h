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
#ifndef AGENT_PROTO_H
#define AGENT_PROTO_H

#include "Agent.pb.h"

namespace MSF {

#define AGENT_VERSION (0x0100)
#define AGENT_VERINFO "libagent-1.0"
#define AGENT_MAGIC 0x12345678

#define AGENT_HEAD_LEN (27) /* Encode size in protobuf */

/**
 * Message header after decode protobuf AgentBhs
 **/
struct tagAgentBhs {
  uint8_t version_;   /* Protocol version - 1 bit */
  uint8_t encrypt_;   /* Header encrypt alg - 1 bit */
  uint16_t magic_;    /* Header magic - 2 bit */
  uint16_t checksum_; /* Header checksum */
  uint16_t cmd_;      /* User command */
  uint32_t bodyLen_;  /* Extra body data length */
  uint32_t sessNo_;   /* Session number */

  tagAgentBhs()
      : version_(0),
        encrypt_(0),
        magic_(0),
        checksum_(0),
        cmd_(0),
        bodyLen_(0),
        sessNo_(0) {}
} __attribute_packed__;

const uint32_t kAgentMagic = 0xab00;
const uint32_t kAgentVersion = 0x0001;
const uint32_t kAgentEncryptZip = 0x0010;
const uint32_t kAgentEncryptGZip = 0x0020;
const uint32_t kAgentEncryptAES = 0x0090;
const uint32_t kAgentEncryptRC5 = 0x00a0;

class AgentProto {
 public:
  AgentProto() {}
  ~AgentProto() {}

  const uint16_t magic(const Agent::AgentBhs &bhs) const;
  const uint8_t version(const Agent::AgentBhs &bhs) const;
  const uint8_t encrypt(const Agent::AgentBhs &bhs) const;
  const uint32_t checkSum(const Agent::AgentBhs &bhs) const;

  const Agent::AppId srcId(const Agent::AgentBhs &bhs) const;
  const Agent::AppId dstId(const Agent::AgentBhs &bhs) const;
  const uint16_t sessNo(const Agent::AgentBhs &bhs) const;
  const Agent::Errno retCode(const Agent::AgentBhs &bhs) const;

  const uint16_t command(const Agent::AgentBhs &bhs) const;
  const Agent::Opcode opCode(const Agent::AgentBhs &bhs) const;
  const uint32_t pduLen(const Agent::AgentBhs &bhs) const;

  void setMagic(Agent::AgentBhs &bhs, const uint16_t magic = kAgentMagic);
  void setVersion(Agent::AgentBhs &bhs, const uint8_t version = kAgentVersion);
  void setEncrypt(Agent::AgentBhs &bhs,
                  const uint8_t encrypt = kAgentEncryptZip);
  void setCheckSum(Agent::AgentBhs &bhs, const uint32_t checkSum);

  void setSrcId(Agent::AgentBhs &bhs, const Agent::AppId srcId);
  void setDstId(Agent::AgentBhs &bhs, const Agent::AppId dstId);
  void setSessNo(Agent::AgentBhs &bhs, const uint16_t sessNo);
  void setRetCode(Agent::AgentBhs &bhs, const Agent::Errno retcode);

  void setCommand(Agent::AgentBhs &bhs, const Agent::Command cmd);
  void setOpcode(Agent::AgentBhs &bhs, const Agent::Opcode op);
  void setPduLen(Agent::AgentBhs &bhs, const uint32_t pduLen);

  void debugBhs(const Agent::AgentBhs &bhs);

 private:
};

}  // namespace MSF
#endif