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
#include "AgentProto.h"

using namespace MSF::AGENT;

namespace MSF {
namespace AGENT {

const uint16_t AgentProto::magic(const Agent::AgentBhs &bhs) const {
  return (uint16_t)(bhs.verify() >> 48);
}

const uint8_t AgentProto::version(const Agent::AgentBhs &bhs) const {
  return static_cast<uint8_t>(bhs.verify() >> 40);
}

const uint8_t AgentProto::encrypt(const Agent::AgentBhs &bhs) const {
  return static_cast<uint8_t>(bhs.verify() >> 32);
}

const uint32_t AgentProto::checkSum(const Agent::AgentBhs &bhs) const {
  return (uint32_t)(bhs.verify());
}

const Agent::AppId AgentProto::srcId(const Agent::AgentBhs &bhs) const {
  uint16_t sid = (uint16_t)(bhs.router() >> 48);
  return static_cast<Agent::AppId>(sid);
}

const Agent::AppId AgentProto::dstId(const Agent::AgentBhs &bhs) const {
  uint16_t did = (uint16_t)(bhs.router() >> 32);
  return static_cast<Agent::AppId>(did);
}

const uint16_t AgentProto::sessNo(const Agent::AgentBhs &bhs) const {
  return (uint16_t)(bhs.router() >> 16);
}

const Agent::Errno AgentProto::retCode(const Agent::AgentBhs &bhs) const {
  uint16_t err = (uint16_t)(bhs.router());
  return static_cast<Agent::Errno>(err);
}

const uint16_t AgentProto::command(const Agent::AgentBhs &bhs) const {
  return (uint16_t)(bhs.command() >> 48);
}

const Agent::Opcode AgentProto::opCode(const Agent::AgentBhs &bhs) const {
  uint8_t op = (uint8_t)(bhs.command() >> 32);
  return static_cast<Agent::Opcode>(op);
}
const uint32_t AgentProto::pduLen(const Agent::AgentBhs &bhs) const {
  return (uint32_t)(bhs.command());
}

// ab00 0110 0000 0000
void AgentProto::setMagic(Agent::AgentBhs &bhs, const uint16_t magic) {
  uint64_t verify = bhs.verify();
  BIT_CLEAR(verify, (static_cast<uint64_t>(0xff)) << 48);
  BIT_SET(verify, (static_cast<uint64_t>(magic)) << 48);
  bhs.set_verify(verify);
}

void AgentProto::setVersion(Agent::AgentBhs &bhs, const uint8_t version) {
  uint64_t verify = bhs.verify();
  BIT_CLEAR(verify, (static_cast<uint64_t>(0xf)) << 40);
  BIT_SET(verify, (static_cast<uint64_t>(version)) << 40);
  bhs.set_verify(verify);
}

void AgentProto::setEncrypt(Agent::AgentBhs &bhs, const uint8_t encrypt) {
  uint64_t verify = bhs.verify();
  BIT_CLEAR(verify, (static_cast<uint64_t>(0xf)) << 32);
  BIT_SET(verify, static_cast<uint64_t>(encrypt) << 32);
  bhs.set_verify(verify);
}

void AgentProto::setCheckSum(Agent::AgentBhs &bhs, const uint32_t checkSum) {
  uint64_t verify = bhs.verify();
  BIT_CLEAR(verify, (static_cast<uint64_t>(0xffff)) << 32);
  BIT_SET(verify, static_cast<uint64_t>(checkSum) << 16);
  bhs.set_verify(verify);
}

void AgentProto::setSrcId(Agent::AgentBhs &bhs, const Agent::AppId srcId) {
  uint64_t router = bhs.router();
  BIT_CLEAR(router, (static_cast<uint64_t>(0xff)) << 48);
  BIT_SET(router, static_cast<uint64_t>(srcId) << 48);
  bhs.set_router(router);
}
void AgentProto::setDstId(Agent::AgentBhs &bhs, const Agent::AppId dstId) {
  uint64_t router = bhs.router();
  BIT_CLEAR(router, (static_cast<uint64_t>(0xff)) << 32);
  BIT_SET(router, static_cast<uint64_t>(dstId) << 32);
  bhs.set_router(router);
}

void AgentProto::setSessNo(Agent::AgentBhs &bhs, const uint16_t sessNo) {
  uint64_t router = bhs.router();
  BIT_CLEAR(router, (static_cast<uint64_t>(0xff)) << 16);
  BIT_SET(router, static_cast<uint64_t>(sessNo) << 16);
  bhs.set_router(router);
}

void AgentProto::setRetCode(Agent::AgentBhs &bhs, const Agent::Errno retcode) {
  uint64_t router = bhs.router();
  BIT_CLEAR(router, static_cast<uint64_t>(0xff));
  BIT_SET(router, static_cast<uint64_t>(retcode));
  bhs.set_router(router);
}

void AgentProto::setCommand(Agent::AgentBhs &bhs, const Agent::Command cmd) {
  uint64_t command = bhs.command();
  BIT_CLEAR(command, (static_cast<uint64_t>(0xff)) << 48);
  BIT_SET(command, static_cast<uint64_t>(cmd) << 48);
  bhs.set_command(command);
}
void AgentProto::setOpcode(Agent::AgentBhs &bhs, const Agent::Opcode op) {
  uint64_t command = bhs.command();
  BIT_CLEAR(command, (static_cast<uint64_t>(0x1)) << 32);
  BIT_SET(command, static_cast<uint64_t>(op) << 32);
  bhs.set_command(command);
}

void AgentProto::setPduLen(Agent::AgentBhs &bhs, const uint32_t pduLen) {
  uint64_t command = bhs.command();
  BIT_CLEAR(command, (static_cast<uint64_t>(0xffff)));
  BIT_SET(command, static_cast<uint64_t>(pduLen));
  bhs.set_command(command);
}

void AgentProto::debugBhs(const Agent::AgentBhs &bhs) {
  MSF_DEBUG << "Bhs:";
  MSF_DEBUG << "magic: " << std::hex << magic(bhs);
  MSF_DEBUG << "version: " << (uint32_t)version(bhs);
  MSF_DEBUG << "encrypt: " << (uint32_t)encrypt(bhs);
  MSF_DEBUG << "checkSum: " << std::hex << checkSum(bhs);

  MSF_DEBUG << "srcid: " << srcId(bhs);
  MSF_DEBUG << "dstid: " << dstId(bhs);
  MSF_DEBUG << "sessno: " << sessNo(bhs);
  MSF_DEBUG << "retCode: " << retCode(bhs);

  MSF_DEBUG << "command: " << std::hex << command(bhs);
  MSF_DEBUG << "opcode: " << opCode(bhs);
  MSF_DEBUG << "pdulen: " << pduLen(bhs);

  MSF_DEBUG << "command: " << std::hex << bhs.verify();
  MSF_DEBUG << "opcode: " << std::hex << bhs.router();
  MSF_DEBUG << "pdulen: " << std::hex << bhs.command();
}

}  // namespace AGENT
}  // namespace MSF