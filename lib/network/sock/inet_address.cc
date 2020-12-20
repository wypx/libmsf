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
#include "inet_address.h"

#include <butil/logging.h>

using namespace MSF;

namespace MSF {

static const in_addr_t g_any4IpAddr = INADDR_ANY;
static const in_addr_t g_anyLoopIpAddr = INADDR_LOOPBACK;
static const in6_addr_t g_any6IpAddr = IN6ADDR_ANY_INIT;

InetAddress::InetAddress()
    : _host(DEFAULT_IPANY),
      _port(DEFAULT_PORT),
      _family(AF_INET),
      _type(0),
      _proto(SOCK_STREAM) {}
InetAddress::InetAddress(const std::string &host, const uint16_t port,
                         const sa_family_t family, const int proto)
    : _host(host), _port(port), _family(family) {
  struct in_addr addr4 = {0};
  struct in6_addr addr6 = IN6ADDR_ANY_INIT;

  if (inet_pton(AF_INET, host.c_str(), &addr4)) {
    sockaddr_in sock_addr = {0};
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr = addr4;
    memcpy(&sock_addr.sin_addr, &addr4, sizeof addr4);
    sock_addr.sin_port = htons(_port);
    initAddress((sockaddr *)&sock_addr);
  } else if (inet_pton(AF_INET6, host.c_str(), &addr6)) {
    sockaddr_in6 sock_addr = {0};
    sock_addr.sin6_family = AF_INET6;
    memcpy(&sock_addr.sin6_addr, &addr6, sizeof addr6);
    sock_addr.sin6_port = htons(_port);
    sock_addr.sin6_scope_id = 64;
    initAddress((sockaddr *)&sock_addr);
  } else if (family == AF_UNIX || family == AF_LOCAL) {
    if (host.size() > sizeof(_addrun.sun_path)) {
      LOG(ERROR) << "Unix path: " << host << " is too long : " << host.size();
    } else {
      memcpy(_addrun.sun_path, host.c_str(), host.size());
      _addrun.sun_family = AF_UNIX;
      // _addrun.sun_len = host.size();
    }
  } else {
    LOG(ERROR) << "Unknown host : " << host;
    sockaddr sock_addr = {0};
    sock_addr.sa_family = AF_UNSPEC;
    initAddress((sockaddr *)&sock_addr);
  }
  _proto = proto;
}
InetAddress::InetAddress(const struct sockaddr *addr) { initAddress(addr); }
InetAddress::InetAddress(const struct sockaddr_in &addr) : _addr4(addr) {
  initAddress((sockaddr *)&addr);
}

InetAddress::InetAddress(const struct sockaddr_in6 &addr) : _addr6(addr) {
  initAddress((sockaddr *)&addr);
}

InetAddress::InetAddress(const struct sockaddr_un &addr) : _addrun(addr) {}

InetAddress::InetAddress(const struct sockaddr_storage &addr) : _addr(addr) {}

InetAddress::InetAddress(const struct in_addr &inaddr) {
  sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr = inaddr;
  memcpy(&addr.sin_addr, &inaddr, sizeof inaddr);
  initAddress((sockaddr *)&addr);
}

InetAddress::InetAddress(const struct in6_addr &in6addr) {
  sockaddr_in6 addr6 = {0};
  addr6.sin6_family = AF_INET6;
  memcpy(&addr6.sin6_addr, &in6addr, sizeof in6addr);
  initAddress((sockaddr *)&addr6);
}

static const char kWellKnownNat64Prefix[] = {'6', '4', ':', 'f', 'f',
                                             '9', 'b', ':', ':', '\0'};

const char *InetAddress::ip() const {
  if (AF_INET == _sa.sa_family) {
    return _ip;
  } else if (AF_INET6 == _sa.sa_family) {
    if (0 == strncasecmp("::FFFF:", _ip, 7))
      return _ip + 7;
    else if (0 == strncasecmp(kWellKnownNat64Prefix, _ip, 9))
      return _ip + 9;
    else
      return _ip;
  }

  // xerror2(TSF"invalid ip family:%_, ip:%_", addr_.sa.sa_family, ip_);
  return "";
}

void InetAddress::initAddress(const struct sockaddr *addr) {
  memset(_ip, 0, sizeof(_ip));
  memset(_url, 0, sizeof(_url));

  if (AF_INET == addr->sa_family) {
    (sockaddr_in &)_addr4 = *(sockaddr_in *)addr;
    inet_ntop(_addr4.sin_family, &(_addr4.sin_addr), _ip, sizeof(_ip));
    snprintf(_url, sizeof(_url), "%s:%u", _ip, _port);
  } else if (AF_INET6 == addr->sa_family) {
    (sockaddr_in6 &)_addr6 = *(sockaddr_in6 *)addr;
    inet_ntop(_addr6.sin6_family, &(_addr6.sin6_addr), _ip, sizeof(_ip));
    snprintf(_url, sizeof(_url), "[%s]:%u", _ip, _port);
  } else {
    _sa.sa_family = AF_UNSPEC;
  }
}

uint16_t InetAddress::sockStorage2Port(const struct sockaddr_storage &addr) {
  uint16_t port;
  if (addr.ss_family == AF_INET6) {
    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&addr;
    port = addr6->sin6_port;
  } else {
    /* Note: this might be AF_UNSPEC if it is the sequence number of
     * a virtual server in a virtual server group */
    struct sockaddr_in *addr4 = (struct sockaddr_in *)&addr;
    port = addr4->sin_port;
  }
  return port;
}

bool InetAddress::isLogalIPAddr(const std::string &host, int family) {
  char addr[sizeof(struct in6_addr)];
#ifdef WIN32
  int len = sizeof(addr);
  if (WSAStringToAddress(host.c_str(), family, NULL, PADDR(addr), &len) == 0) {
    return true;
  }
#else  /*~WIN32*/
  if (inet_pton(family, host.c_str(), addr) == 1) {
    return true;
  }
#endif /*WIN32*/
  return false;
}

bool InetAddress::sockAddrCmp(const struct sockaddr *sa1,
                              const struct sockaddr *sa2) {
  if (sa1->sa_family != sa2->sa_family) return false;

  if (sa1->sa_family == AF_INET) {
    const struct sockaddr_in *sin1, *sin2;
    sin1 = (const struct sockaddr_in *)sa1;
    sin2 = (const struct sockaddr_in *)sa2;

    if ((sin1->sin_addr.s_addr == sin2->sin_addr.s_addr) &&
        ((int)sin1->sin_port == (int)sin2->sin_port)) {
      return true;
    }
    return false;
  }
#ifdef AF_INET6
  else if (sa1->sa_family == AF_INET6) {
    const struct sockaddr_in6 *sin1, *sin2;
    sin1 = (const struct sockaddr_in6 *)sa1;
    sin2 = (const struct sockaddr_in6 *)sa2;
    if ((0 == memcmp(sin1->sin6_addr.s6_addr, sin2->sin6_addr.s6_addr, 16)) &&
        ((int)sin1->sin6_port == (int)sin2->sin6_port))
      return true;
    else
      return true;
  }
#endif
  return false;
}

std::string InetAddress::parseProtoName(int proto) {
  std::string protoName;
#if 0
    switch (proto) {
        case 0:
            protoName = "";
            break;
        case IPPROTO_TCP:
            protoName = "tcp";
            break;
        case IPPROTO_UDP:
            protoName = "udp";
            break;
#ifdef IPPROTO_SCTP
        case IPPROTO_SCTP:
            protoName = "sctp";
            break;
#endif
        default:
#ifdef HAVE_GETPROTOBYNUMBER
            {
            struct protoent *ent = getprotobynumber(proto);
            if (ent)
                protoName = ent->p_name;
            }
#endif
    }
#endif
  return protoName;
}

void InetAddress::setIPv6ScopeId(uint32_t scope_id) {
  if (family() == AF_INET6) {
    _addr6.sin6_scope_id = scope_id;
  }
}

}  // namespace MSF