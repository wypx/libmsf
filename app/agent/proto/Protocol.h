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
#ifndef __MSF_PROTOCOL_H__
#define __MSF_PROTOCOL_H__

#include <base/GccAttr.h>

#include <stdint.h>

using namespace MSF::BASE;

namespace MSF {
namespace AGENT {

#ifdef __cplusplus
extern "C" {
#endif

#define AGENT_VERSION       (0x0100) 
#define AGENT_VERINFO       "libagent-1.0"
#define AGENT_MAGIC         0x12345678

#define AGENT_KEEP_ALIVE_SECS 60
#define AGENT_KEEP_ALIVE_TIMEOUT_SECS (7*60+1) /* has 7 tries to send a keep alive */


/* Note: 0x1 - 0x10 is reseved id */

enum AgentAppId {
    APP_AGENT    = 0,
    APP_GUARD    = 1,
    APP_MOBILE   = 2,
    APP_DLNA     = 3,
    APP_UPNP     = 4,
    APP_DDNS     = 5
};

enum AgentErrno {
    AGENT_E_EXEC_SUCESS         = 0,
    AGENT_E_EXEC_FAILURE        = 1,
    AGENT_E_LOGIN_SUCESS        = 2,
    AGENT_E_LOGIN_FAILURE       = 3,
    AGENT_E_LOGIN_UNAUTH        = 4,
    AGENT_E_PEER_OFFLINE        = 5,
    AGENT_E_SEND_TIMEROUT       = 6,
    AGENT_E_RECV_TIMEROUT       = 7
};

enum AgentCommand {
    AGENT_LOGIN_REQUEST         = 0,
    AGENT_LOGIN_RESPONSE        = 1,
    AGENT_LOGOUT_REQUEST        = 2,
    AGENT_LOGOUT_RESPONSE       = 3,
    AGENT_NOPIN_REQUEST         = 4,
    AGENT_NOPIN_REPONSE         = 5,
    AGENT_READ_REQUEST          = 6,
    AGENT_READ_RESPONCE         = 7,
    AGENT_WRITE_REQUEST         = 8,
    AGENT_WRITE_RESPONCE        = 9,
    AGENT_DEBUG_ON_REQUEST      = 10,
    AGENT_DEBUG_ON_RESPONCE     = 11,
    AGENT_DEBUG_OFF_REQUEST     = 12,
    AGENT_DEBUG_OFF_RESPONCE    = 13
};

enum AgentPacket {
    AGENT_PACKET_BINNARY    = 0,
    AGENT_PACKET_JSON       = 1,
    AGENT_PACKET_PROTOBUF   = 2,
    AGENT_PACKET_BUTT       = 3
};

struct AgentBhs {
    uint32_t version_;/* high 8 major ver, low 8 bug and func update */
    uint32_t magic_;  /* Assic:U:0x55 I:0x49 P:0x50 C:0x43 */
    enum AgentAppId srcId_;
    enum AgentAppId dstId_;
    enum AgentCommand cmd_;
    enum AgentErrno retCode_;
    uint32_t sessNo_;
    uint32_t dataLen_;
    uint32_t restLen_;
    uint32_t checkSum_;   /* Message Header checksum */
    uint32_t timeOut_;    /* Timeout wait for data */
    uint8_t  reserved_[8];
} MSF_PACKED_MEMORY;


struct AgentLogin {
    uint8_t  name_[32];
    uint8_t  hash_[32];/* name and pass do hash */
    bool     chap_;
} MSF_PACKED_MEMORY;

struct AgentPdu {
    enum AgentAppId   dstId_;
    enum AgentCommand cmd_;
    uint32_t    timeOut_;
    uint8_t     *payLoad_;
    uint32_t    payLen_;
    uint8_t     *restLoad_;
    uint32_t    restLen_;
} MSF_PACKED_MEMORY;

#ifdef __cplusplus
}
#endif

}
}
#endif
