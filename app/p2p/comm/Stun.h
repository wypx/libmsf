
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
#ifndef P2P_COMM_STUN_H
#define P2P_COMM_STUN_H

#include <string>

namespace MSF {
namespace P2P {

const std::string StunSoftware = "P2P-Luotang";

/* STUN Reason Phrase */
const std::string StunReason300 = "Try Alternate";
const std::string StunReason400 = "Bad Request";
const std::string StunReason401 = "Unauthorized";
const std::string StunReason403 = "Forbidden";
const std::string StunReason420 = "Unknown Attribute";
const std::string StunReason437 = "Allocation Mismatch";
const std::string StunReason438 = "Stale Nonce";
const std::string StunReason440 = "Address Family not Supported";
const std::string StunReason441 = "Wrong Credentials";
const std::string StunReason442 = "Unsupported Transport Protocol";
const std::string StunReason443 = "Peer Address Family Mismatch";
const std::string StunReason486 = "Allocation Quota Reached";
const std::string StunReason500 = "Server Error";
const std::string StunReason508 = "Insufficient Capacity";

enum {
	STUN_MAGIC_COOKIE = 0x2112a442 /**< Magic Cookie for 3489bis        */
};

/** Calculate STUN message type from method and class */
#define STUN_TYPE(method, class)      \
  ((method)&0x0f80) << 2 |      \
  ((method)&0x0070) << 1 |      \
  ((method)&0x000f) << 0 |      \
  ((class)&0x2)     << 7 |      \
  ((class)&0x1)     << 4


#define STUN_CLASS(type) \
	((type >> 7 | type >> 4) & 0x3)


#define STUN_METHOD(type) \
	((type&0x3e00)>>2 | (type&0x00e0)>>1 | (type&0x000f))

/** STUN Methods */
enum StunMethod {
  STUN_METHOD_BINDING         = 0x001, // STUN Binding method as defined by RFC 3489-bis.
  STUN_METHOD_SHARED_SECRET   = 0x002, // STUN Shared Secret method as defined by RFC 3489-bis.
  STUN_METHOD_ALLOCATE   = 0x003, // STUN/TURN Allocate method as defined by draft-ietf-behave-turn
  STUN_METHOD_REFRESH    = 0x004, // STUN/TURN Refresh method as defined by draft-ietf-behave-turn
  STUN_METHOD_SEND       = 0x006, // STUN/TURN Send indication as defined by draft-ietf-behave-turn
  STUN_METHOD_DATA       = 0x007, // STUN/TURN Data indication as defined by draft-ietf-behave-turn
  STUN_METHOD_CREATEPERM = 0x008, // STUN/TURN CreatePermission method as defined by draft-ietf-behave-turn
  STUN_METHOD_CHANBIND   = 0x009, // STUN/TURN ChannelBind as defined by draft-ietf-behave-turn
};

/** STUN Message class */
enum StunClass {
  STUN_CLASS_REQUEST      = 0x0,  /**< STUN Request          */
  STUN_CLASS_INDICATION   = 0x1,  /**< STUN Indication       */
  STUN_CLASS_SUCCESS_RESP = 0x2,  /**< STUN Success Response */
  STUN_CLASS_ERROR_RESP   = 0x3   /**< STUN Error Response   */
};

/** STUN Attributes */
enum StunAttrib {
  /* Comprehension-required range (0x0000-0x7FFF) */
  STUN_ATTR_MAPPED_ADDR	    = 0x0001,/**< MAPPED-ADDRESS.	    */
  STUN_ATTR_RESPONSE_ADDR	    = 0x0002,/**< RESPONSE-ADDRESS (deprcatd)*/
  STUN_ATTR_CHANGE_REQUEST	    = 0x0003,/**< CHANGE-REQUEST (deprecated)*/
  STUN_ATTR_SOURCE_ADDR	    = 0x0004,/**< SOURCE-ADDRESS (deprecated)*/
  STUN_ATTR_CHANGED_ADDR	    = 0x0005,/**< CHANGED-ADDRESS (deprecatd)*/
  STUN_ATTR_USERNAME	    = 0x0006,/**< USERNAME attribute.	    */
  STUN_ATTR_PASSWORD	    = 0x0007,/**< was PASSWORD attribute.   */
  STUN_ATTR_MESSAGE_INTEGRITY  = 0x0008,/**< MESSAGE-INTEGRITY.	    */
  STUN_ATTR_ERROR_CODE	    = 0x0009,/**< ERROR-CODE.		    */
  STUN_ATTR_UNKNOWN_ATTRIBUTES = 0x000A,/**< UNKNOWN-ATTRIBUTES.	    */
  STUN_ATTR_REFLECTED_FROM	    = 0x000B,/**< REFLECTED-FROM (deprecatd)*/
  STUN_ATTR_CHANNEL_NUMBER	    = 0x000C,/**< TURN CHANNEL-NUMBER	    */
  STUN_ATTR_LIFETIME	    = 0x000D,/**< TURN LIFETIME attr.	    */
  STUN_ATTR_MAGIC_COOKIE	    = 0x000F,/**< MAGIC-COOKIE attr (deprec)*/
  STUN_ATTR_BANDWIDTH	    = 0x0010,/**< TURN BANDWIDTH (deprec)   */
  STUN_ATTR_XOR_PEER_ADDR	    = 0x0012,/**< TURN XOR-PEER-ADDRESS	    */
  STUN_ATTR_DATA		    = 0x0013,/**< DATA attribute.	    */
  STUN_ATTR_REALM		    = 0x0014,/**< REALM attribute.	    */
  STUN_ATTR_NONCE		    = 0x0015,/**< NONCE attribute.	    */
  STUN_ATTR_XOR_RELAYED_ADDR   = 0x0016,/**< TURN XOR-RELAYED-ADDRESS  */
  STUN_ATTR_REQ_ADDR_TYPE	    = 0x0017,/**< REQUESTED-ADDRESS-TYPE    */
  STUN_ATTR_REQ_ADDR_FAMILY    = 0x0017,/**< REQUESTED-ADDRESS-FAMILY  */
  STUN_ATTR_EVEN_PORT	    = 0x0018,/**< TURN EVEN-PORT	    */
  STUN_ATTR_REQ_TRANSPORT	    = 0x0019,/**< TURN REQUESTED-TRANSPORT  */
  STUN_ATTR_DONT_FRAGMENT	    = 0x001A,/**< TURN DONT-FRAGMENT	    */
  STUN_ATTR_XOR_MAPPED_ADDR    = 0x0020,/**< XOR-MAPPED-ADDRESS	    */
  STUN_ATTR_TIMER_VAL	    = 0x0021,/**< TIMER-VAL attribute.	    */
  STUN_ATTR_RESERVATION_TOKEN  = 0x0022,/**< TURN RESERVATION-TOKEN    */
  STUN_ATTR_XOR_REFLECTED_FROM = 0x0023,/**< XOR-REFLECTED-FROM	    */
  STUN_ATTR_PRIORITY	    = 0x0024,/**< PRIORITY		    */
  STUN_ATTR_USE_CANDIDATE	    = 0x0025,/**< USE-CANDIDATE		    */
  STUN_ATTR_PADDING            = 0x0026,
  STUN_ATTR_RESP_PORT          = 0x0027,
  STUN_ATTR_CONNECTION_ID	    = 0x002a,/**< CONNECTION-ID		    */
  STUN_ATTR_ICMP		    = 0x0030,/**< ICMP (TURN)		    */

  STUN_ATTR_END_MANDATORY_ATTR,

  STUN_ATTR_START_EXTENDED_ATTR= 0x8021,

  /* Comprehension-optional range (0x8000-0xFFFF) */
  STUN_ATTR_SOFTWARE	    = 0x8022,/**< SOFTWARE attribute.	    */
  STUN_ATTR_ALTERNATE_SERVER   = 0x8023,/**< ALTERNATE-SERVER.	    */
  STUN_ATTR_REFRESH_INTERVAL   = 0x8024,/**< REFRESH-INTERVAL.	    */
  STUN_ATTR_FINGERPRINT	    = 0x8028,/**< FINGERPRINT attribute.    */
  STUN_ATTR_ICE_CONTROLLED	    = 0x8029,/**< ICE-CCONTROLLED attribute.*/
  STUN_ATTR_ICE_CONTROLLING    = 0x802a,/**< ICE-CCONTROLLING attribute*/
  STUN_ATTR_RESP_ORIGIN        = 0x802b,
  STUN_ATTR_OTHER_ADDR         = 0x802c,
  STUN_ATTR_END_EXTENDED_ATTR
};

#define STUN_MAX_STRING             (256)
#define STUN_MAX_UNKNOWN_ATTRIBUTES (8)
#define STUN_MAX_MESSAGE_SIZE       (2048)

struct StunAttrChangeRequest {
  uint32_t value_;
};

struct StunAttrError {
  uint16_t pad_;                 // all 0
  uint8_t errorClass_;
  uint8_t number_;
  char reason_[STUN_MAX_STRING];
  uint16_t sizeReason_;
};

struct StunAttrUnknown {
  uint16_t attrType_[STUN_MAX_UNKNOWN_ATTRIBUTES];
  uint16_t numAttr_;
};

struct StunAttrString {
  char value_[STUN_MAX_STRING];
  uint16_t sizeValue_;
};

struct StunAttrIntegrity {
  char hash_[20];
};

enum {
  TURN_DEFAULT_LIFETIME = 600,  /**< Default lifetime is 10 minutes */
  TURN_MAX_LIFETIME     = 3600  /**< Maximum lifetime is 1 hour     */
};

enum {
  STUN_PORT        = 3478,   /**< STUN Port number */
  STUNS_PORT       = 5349,   /**< STUNS Port number                    */
  STUN_HEADER_SIZE = 20,     /**< Number of bytes in header            */
  STUN_ATTR_HEADER_SIZE = 4, /**< Size of attribute header             */
  STUN_TID_SIZE    = 12,     /**< Number of bytes in transaction ID    */
  STUN_DEFAULT_RTO = 500,    /**< Default Retrans Timeout in [ms]      */
  STUN_DEFAULT_RC  = 7,      /**< Default number of retransmits        */
  STUN_DEFAULT_RM  = 16,     /**< Wait time after last request is sent */
  STUN_DEFAULT_TI  = 39500   /**< Reliable timeout */
};

enum TurnTransType {
    TURN_TP_UDP = 17, /* UDP transport, which value corresponds to IANA protocol number. */
    TURN_TP_TCP = 6, /* TCP transport, which value corresponds to IANA protocol number. */
    /* TLS transport. The TLS transport will only be used as the connection
     * type to reach the server and never as the allocation transport type.
     * The value corresponds to IANA protocol number.*/
    TURN_TP_TLS = 56
};

struct StunHdr {
  uint16_t type_;               /**< Message type   */
  uint16_t len_;                /**< Payload length */
  uint32_t cookie_;             /**< Magic cookie   */
  uint8_t tid_[STUN_TID_SIZE];  /**< Transaction ID */
};

struct StunAddr {
    uint16_t port_;
    uint32_t addr_;
};

struct NatInfo {
    int fd;
    int type;
    struct StunAddr local;
    struct StunAddr reflect;
    uint32_t uuid;
};

class Stun {
  private:

  public:
    bool init(const std::string & ip);
    bool socket(const std::string & ip, const uint16_t port, StunAddr *map);
    const enum StunNatType natType() const;
    void debugNatType(const enum StunNatType type);
    void keepAlive(const int fd);
};


}
}
#endif