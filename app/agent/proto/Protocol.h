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

#define RPC_VERSION     (0x0100) 
#define RPC_VERINFO     "libagent-1.0"

#define RPC_REQ         0x1
#define RPC_ACK         0x2
#define RPC_CANCEL      0x3
#define RPC_KALIVE      0x4

#define RPC_MAGIC       0x12345678

/* Note: 0x1 - 0x10 is reseved id */
#define RPC_DLNA_ID     0x11
#define RPC_UPNP_ID     0x12
#define RPC_HTTP_ID     0x13
#define RPC_DDNS_ID     0x14
#define RPC_SMTP_ID     0x15
#define RPC_STUN_ID     0x16
#define RPC_MOBILE_ID   0x17
#define RPC_STORAGE_ID  0x18
#define RPC_DATABASE_ID 0x19

/* */
#define RPC_MSG_SRV_ID   0xfffffff1
#define RPC_LOGIN_SRV_ID 0xfffffff2
#define RPC_IMAGE_SRV_ID 0xfffffff3

#define RPC_KEEP_ALIVE_SECS 60
#define RPC_KEEP_ALIVE_TIMEOUT_SECS (7*60+1) /* has 7 tries to send a keep alive */

enum RPC_ERRCODE_ID {
    RPC_EXEC_SUCC       = 0,
    RPC_LOGIN_FAIL      = 1,
    RPC_PEER_OFFLINE    = 2,
};

enum opcode {
    /* Client to Server Message Opcode values */
    op_nopout_cmd   = 0x01,
    op_login_cmd    = 0x02,
    op_data_cmd     = 0x03,

    /* Server to Client Message Opcode values */
    op_nopin_cmd    = 0x10,
    op_login_rsp    = 0x11,
    op_data_rsp     = 0x12,
};

/* Message types */
enum RPC_COMMAND_ID {
    /* Command for rpc daemon process,
     * such as subscibe messages */
    RPC_LOGIN           = 0x01,
    RPC_LOGOUT          = 0x02,
    RPC_TRANSMIT        = 0x03,
    RPC_DEBUG_ON        = 0x04,
    RPC_DEBUG_OFF       = 0x05,

    /* Command for plugins and compoments */
    MOBILE_SET_PARAM    = 0x10,
    MOBILE_GET_PARAM    = 0x11,

    SMTP_SET_PARAM      = 0x21,
    SMTP_GET_PARAM      = 0x22,
    DDNS_SET_PARAM      = 0x23,
    DDNS_GET_PARAM      = 0x24,
    UPNP_SET_PARAM      = 0x25,
    UPNP_GET_PARAM      = 0x26,
    DLNA_SET_PARAM      = 0x27,
    DLNA_GET_PARAM      = 0x28,
    STUN_SET_PARAM      = 0x29,
    STUN_GET_PARAM      = 0x2a,
};

enum pack_type {
    PACK_BINARY,
    PACK_JSON,
    PACK_PROTO,
    PACK_BUTT,
};

struct AgentBhs {
    uint32_t version;/* high 8 major ver, low 8 bug and func update */
    uint32_t magic;  /* Assic:U:0x55 I:0x49 P:0x50 C:0x43 */

    uint32_t srcid;
    uint32_t dstid;
    uint32_t opcode;
    uint32_t cmd;
    uint32_t seq;
    uint32_t errcode;

    uint32_t datalen;
    uint32_t restlen;

    uint32_t checksum;   /* Message Header checksum */
    uint32_t timeout;    /* Timeout wait for data */

    uint8_t  reserved[8];
} MSF_PACKED_MEMORY;


struct AgentLogin {
    uint8_t  name[32];
    uint8_t  hash[32];/* name and pass do hash */
    bool     chap;
} MSF_PACKED_MEMORY;

struct AgentPdu {
    uint32_t    dstid;
    uint32_t    cmd;
    uint32_t    timeout;
    uint8_t     *payload;
    uint32_t    paylen;
    uint8_t     *restload;
    uint32_t    restlen;
} MSF_PACKED_MEMORY;

#ifdef __cplusplus
}
#endif

}
}
#endif
