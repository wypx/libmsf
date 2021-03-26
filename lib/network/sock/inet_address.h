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
#ifndef SOCK_INETADDRESS_H_
#define SOCK_INETADDRESS_H_

#include <arpa/inet.h>
#include <linux/in6.h>
#include <net/ethernet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <iostream>

#include "noncopyable.h"

using namespace MSF;

namespace MSF {

#define SIN(sa) ((struct sockaddr_in*)sa)
#define SIN6(sa) ((struct sockaddr_in6*)sa)
#define SINU(sa) ((struct sockaddr_un*)sa)
#define PADDR(a) ((struct sockaddr*)a)

/*
 * IPv6 address
 */
typedef struct in6_addr {
  union {
    __uint8_t __u6_addr8[16];
    __uint16_t __u6_addr16[8];
    __uint32_t __u6_addr32[4];
  } __u6_addr; /* 128-bit IP6 address */
} in6_addr_t;

/*
 * Argument structure for IP_ADD_MEMBERSHIP and IP_DROP_MEMBERSHIP.
 */
struct ip_mreq {
  struct in_addr imr_multiaddr; /* IP multicast address of group */
  struct in_addr imr_interface; /* local IP address of interface */
};

/*
 * Argument structure for IPV6_JOIN_GROUP and IPV6_LEAVE_GROUP.
 */
struct ipv6_mreq {
  struct in6_addr ipv6mr_multiaddr;
  unsigned int ipv6mr_interface;
};

#define DEFAULT_IPV4_ADDR "127.0.0.1"
#define DEFAULT_IPV6_ADDR "::1"
#define DEFAULT_IPANY "0.0.0.0"
#define DEFAULT_PORT 8888

#define DEFAULT_SRVAGENT_UNIXPATH "/var/tmp/AgentServer.sock"
#define DEFAULT_CLIAGENT_UNIXPATH "/var/tmp/AgentClient.sock"

class InetAddress : public copyable {
 public:
  enum NetworkType {
    UNIX,
    TCP,
    UDP,
    SCTP,
    NETLINK,
    MUTICAST,
    Undefined
  };
  enum AddrFamily {
    IPV4,
    IPV6,
    UNKNOWN
  };

 public:
  explicit InetAddress(const std::string& host = DEFAULT_IPANY,
                       const uint16_t port = DEFAULT_PORT,
                       const sa_family_t family = AF_INET,
                       const int proto = AF_INET);
  explicit InetAddress(const struct sockaddr* addr);
  explicit InetAddress(const struct sockaddr_in& addr);
  explicit InetAddress(const struct sockaddr_in6& addr);
  explicit InetAddress(const struct sockaddr_un& addr);
  explicit InetAddress(const struct sockaddr_storage& addr);
  explicit InetAddress(const struct in_addr& inaddr);
  explicit InetAddress(const struct in6_addr& in6addr);

  /* ipport ipv4/ipv6 address format like "127.0.0.1:8000" */
  InetAddress(const std::string& ip, const uint16_t port);
  InetAddress(const std::string& ipport);

  ~InetAddress() = default;

  // https://blog.csdn.net/lxj434368832/article/details/78874108
  std::string hostPort2String() const {
    return host_ + ":" + std::to_string(port_);
  }
  std::string host() const { return host_; }
  uint16_t port() const {
    if (AF_INET == _sa.sa_family) {
      return ntohs(_addr4.sin_port);
    } else if (AF_INET6 == _sa.sa_family) {
      return ntohs(_addr6.sin6_port);
    }
    return 0;
  }

  int family() const { return family_; }

  int proto() const { return proto_; }

  AddrFamily net_type() const { return net_type_; }

  const char* ip() const;

  // struct sockaddr* getSockAddr() const { return _addr4; }
  void setSockAddrInet4(const struct sockaddr_in& addr4) { _addr4 = addr4; }
  void setSockAddrInet6(const struct sockaddr_in6& addr6) { _addr6 = addr6; }
  void setSockAddrUnix(const struct sockaddr_un& addrun) { _addrun = addrun; }
  void setSockAddrStorage(const struct sockaddr_storage& addr) { _addr = addr; }

  void setIPv6ScopeId(uint32_t scope_id);

  inline friend bool operator==(const InetAddress& a, const InetAddress& b) {
    return a._addr4.sin_family == b._addr4.sin_family &&
           a._addr4.sin_addr.s_addr == b._addr4.sin_addr.s_addr &&
           a._addr4.sin_port == b._addr4.sin_port;
  }

  inline friend bool operator!=(const InetAddress& a, const InetAddress& b) {
    return !(a == b);
  }

 private:
  std::string host_;
  uint16_t port_;
  int family_;
  int proto_;
  AddrFamily net_type_;

  union {
    struct sockaddr _sa;
    struct sockaddr_in _addr4;
    struct sockaddr_in6 _addr6;
    struct sockaddr_un _addrun;
    struct sockaddr_storage _addr;
  };
  char _ip[96];
  char _url[128];

  void initAddress(const struct sockaddr* addr);
  bool isLogalIPAddr(const std::string& host, int family);
  uint16_t sockStorage2Port(const struct sockaddr_storage& addr);
  bool sockAddrCmp(const struct sockaddr* sa1, const struct sockaddr* sa2);
  std::string parseProtoName(int proto);
};

}  // namespace MSF
#endif