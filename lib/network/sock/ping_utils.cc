/**************************************************************************
 *
 * Copyright (c) 2017-2018, luotang.me <wypx520@gmail.com>, China.
 * All rights reserved.
 *
 * Distributed under the terms of the GNU General Public License v2.
 *
 * This software is provided 'as is' with no explicit or implied warranties
 * in respect of its properties, including, but not limited to, correctness
 * and/or fitness for purpose.
 *
 **************************************************************************/
#include "ping_utils.h"

#include <arpa/inet.h>
#include <base/logging.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netdb.h>
#include <netinet/if_ether.h>
#include <netinet/ip_icmp.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "inet_address.h"
#include "sock_utils.h"

using namespace MSF;

namespace MSF {

#define PING_PERM_ERROR -99
#define PING_DNS_FAILURE -1
#define PING_ERROR -2
#define PING_FAILED 0
#define PING_SUCCESS 1

#define STR_PING_PERM_ERROR \
  "Ping failed. Permission error. (Not running as root?)"
#define STR_PING_DNS_FAILURE \
  "Ping failed. DNS error. Cannot map hostname to IP address."
#define STR_PING_ERROR                                                  \
  "Ping error. Cannot ping host. Connection blocked or ping sent only " \
  "partially."
#define STR_PING_FAILED "Ping failed. Host not reached."
#define STR_PING_SUCCESS "Ping success. Host reached."
#define STR_PING_UNKNOWN "Unknown result for ping function."

#define DATALEN 56
#define IPADDRLEN 60
#define ICMPLEN 76

/* common routines */
static int in_cksum(uint16_t *buf, uint32_t sz) {
  uint32_t nleft = sz;
  uint32_t sum = 0;
  uint16_t *w = buf;
  uint16_t ans = 0;

  while (nleft > 1) {
    sum += *w++;
    nleft -= 2;
  }

  if (nleft == 1) {
    *(uint8_t *)(&ans) = *(uint8_t *)w;
    sum += ans;
  }

  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);
  ans = ~sum;
  return (ans);
}

void tv_sub(struct timeval *out, struct timeval *in) {
  if ((out->tv_usec -= in->tv_usec) < 0) {
    --out->tv_sec;
    out->tv_usec += 1000000;
  }
  out->tv_sec -= in->tv_sec;
}

// http://blog.sina.com.cn/s/blog_5d0d91ba0100v8j1.html
int Ping(const char *host, int ping_timeout) {
  struct hostent *h;
  struct sockaddr_in pingaddr;
  struct sockaddr_in *sin = nullptr;
  struct sockaddr_in6 *sin6 = nullptr;
  struct icmp *packet;
  int pingsock, c;
  char pkt[DATALEN + IPADDRLEN + ICMPLEN];

  pingsock = CreateICMPSocket();
  if (pingsock < 0) {
    // Error, cannot create socket (not running as root?)
    return (PING_PERM_ERROR);
  }

  // 设置接收超时
  SetTimeout(pingsock, 2);

  // getuid()函数返回一个调用程序的真实用户ID
  // setuid()是让普通用户可以以root用户的角色运行
  // 只有root帐号才能运行的程序或命令
  if (::setuid(getuid()) < 0) {
    return -1;
  }

  ::memset(&pingaddr, 0, sizeof(struct sockaddr_in));
  pingaddr.sin_family = AF_INET;
  if (!(h = ::gethostbyname(host)))
    return (PING_DNS_FAILURE);  // Unknown host, dns failed?

  ::memcpy(&pingaddr.sin_addr, h->h_addr, sizeof(pingaddr.sin_addr));

  packet = (struct icmp *)pkt;
  ::memset(packet, 0, sizeof(struct icmp));
  packet->icmp_type = ICMP_ECHO;
  packet->icmp_id = ::getpid();
  packet->icmp_cksum = in_cksum((uint16_t *)packet, sizeof(pkt));
  // packet->icmp_seq++;

  struct timeval *tval = (struct timeval *)packet->icmp_data;
  ::gettimeofday(tval, nullptr);

  int af = AF_INET;
  if (af == AF_INET) {
    sin = (struct sockaddr_in *)(&pingaddr);
    sin->sin_family = AF_INET;
    c = ::sendto(pingsock, packet, DATALEN + ICMP_MINLEN, 0,
                 (struct sockaddr *)sin, sizeof(struct sockaddr_in));

  } else if (af == AF_INET6) {
    sin6 = (struct sockaddr_in6 *)(&pingaddr);
    sin6->sin6_family = AF_INET6;
    c = ::sendto(pingsock, packet, DATALEN + ICMP_MINLEN, 0,
                 (struct sockaddr *)sin6, sizeof(struct sockaddr_in6));
  }
  if (c < 0 || c != sizeof(pkt)) {
    CloseSocket(pingsock);
    return (PING_ERROR);  // Partially failed, not able to send complete packet
  }

  time_t timeout = ::time(NULL) + ping_timeout;
  struct timeval tv;  // set timeout 100ms
  tv.tv_sec = 0;
  // tv.tv_usec = 100000;
  tv.tv_usec = 10000 * ping_timeout;
  if (::setsockopt(pingsock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    return -1;
  }

  /* listen for replies */
  while (::time(NULL) < timeout) {
    struct sockaddr_in from;
    struct timeval tv_recv;
    struct timeval *tv_send;
    socklen_t fromlen = sizeof(from);
    if ((c = ::recvfrom(pingsock, pkt, sizeof(pkt), 0, (struct sockaddr *)&from,
                        &fromlen)) < 0)
      continue;
    ::gettimeofday(&tv_recv, nullptr);
    if (c >= ICMPLEN) {
      struct ip *ippkt = (struct ip *)pkt;
      struct iphdr *iphdr = (struct iphdr *)pkt;
      packet = (struct icmp *)(pkt + (iphdr->ihl << 2));
      if (packet->icmp_type == ICMP_ECHOREPLY &&
          packet->icmp_id == ::getpid()) {
        tv_send = (struct timeval *)packet->icmp_data;
        tv_sub(&tv_recv, tv_send);
        double rtt = tv_recv.tv_sec * 1000 + tv_recv.tv_usec / 1000;
        LOG(INFO) << c << " byte from " << ::inet_ntoa(from.sin_addr)
                  << ": icmp_seq=" << packet->icmp_seq
                  << " ttl=" << ippkt->ip_ttl << " rtt=" << rtt << " ms";
        return (PING_SUCCESS);  // Got reply succesfully
      } else if (packet->icmp_type == ICMP_DEST_UNREACH) {
        LOG(ERROR) << "Ping host unreachable.";
        return (PING_ERROR);
      }
    }
  }
  return (PING_FAILED);  // Ping failed
}

std::string PingResult(int result) {
  switch (result) {
    case PING_PERM_ERROR:
      return STR_PING_PERM_ERROR;
      break;
    case PING_DNS_FAILURE:
      return STR_PING_DNS_FAILURE;
      break;
    case PING_ERROR:
      return STR_PING_ERROR;
      break;
    case PING_SUCCESS:
      return STR_PING_SUCCESS;
      break;
    case PING_FAILED:
      return STR_PING_FAILED;
      break;
    default:
      break;
  }
  return STR_PING_UNKNOWN;
}

enum { ARP_MSG_SIZE = 0x2a };

#define MAC_BCAST_ADDR (uint8_t *)"/xff/xff/xff/xff/xff/xff"
#define ETH_INTERFACE "eth0"

struct arpMsg {
  struct ethhdr
      ethhdr; /* Ethernet header u_char h_dest[6] h_source[6]  h_proto     */
  uint16_t htype;     /* hardware type (must be ARPHRD_ETHER) */
  uint16_t ptype;     /* protocol type (must be ETH_P_IP) */
  uint8_t hlen;       /* hardware address length (must be 6) */
  uint8_t plen;       /* protocol address length (must be 4) */
  uint16_t operation; /* ARP packet */
  uint8_t sHaddr[6];  /* sender's hardware address */
  uint8_t sInaddr[4]; /* sender's IP address */
  uint8_t tHaddr[6];  /* target's hardware address */
  uint8_t tInaddr[4]; /* target's IP address */
  uint8_t pad[18];    /* pad for min. Ethernet payload (60 bytes) */
};

uint32_t server = 0;
uint32_t ifindex = -1;
uint8_t arp[6] = {0};
uint16_t conflict_time = 0;

int arpping(uint32_t yiaddr, uint32_t ip, char *mac, char *interface) {
  int timeout = 2;
  int optval = 1;
  int s = -1;           /* socket */
  int rv = 1;           /* return value */
  struct sockaddr addr; /* for interface name */
  struct arpMsg arp;
  fd_set fdset;
  struct timeval tm;
  time_t prevTime;

  if (-1 == (s = ::socket(PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP)))) {
    LOG(ERROR) << "Could not create raw socket.";
    return -1;
  }

  if (-1 ==
      ::setsockopt(s, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval))) {
    LOG(ERROR) << "Could not setsocketopt on raw socket.";
    CloseSocket(s);
    return -1;
  }

  /*http://blog.csdn.net/wanxiao009/archive/2010/05/21/5613581.aspx */
  ::memset(&arp, 0, sizeof(arp));
  ::memcpy(arp.ethhdr.h_dest, MAC_BCAST_ADDR, 6); /* MAC DA */
  ::memcpy(arp.ethhdr.h_source, mac, 6);          /* MAC SA */
  arp.ethhdr.h_proto = htons(ETH_P_ARP);          /* protocol type (Ethernet) */
  arp.htype = htons(ARPHRD_ETHER);                /* hardware type */
  arp.ptype = htons(ETH_P_IP);              /* protocol type (ARP message) */
  arp.hlen = 6;                             /* hardware address length */
  arp.plen = 4;                             /* protocol address length */
  arp.operation = htons(ARPOP_REQUEST);     /* ARP op code */
  ::memcpy(arp.sInaddr, (uint8_t *)&ip, 4); /* source IP address */
  ::memcpy(arp.sHaddr, mac, 6);             /* source hardware address */
  ::memcpy(arp.tInaddr, (uint8_t *)&yiaddr, 4); /* target IP address */

  memset(&addr, 0, sizeof(addr));
  strcpy(addr.sa_data, interface);
  if (::sendto(s, &arp, sizeof(arp), 0, &addr, (socklen_t)sizeof(addr)) < 0)
    rv = 0;

  tm.tv_usec = 0;
  ::time(&prevTime);
  while (timeout > 0) {
    FD_ZERO(&fdset);
    FD_SET(s, &fdset);
    tm.tv_sec = timeout;
    if (::select(s + 1, &fdset, (fd_set *)NULL, (fd_set *)NULL, &tm) < 0) {
      LOG(ERROR) << "Error on ARPING request: " << strerror(errno);
      if (errno != EINTR) rv = 0;
    } else if (FD_ISSET(s, &fdset)) {
      if (recv(s, &arp, sizeof(arp), 0) < 0) rv = 0;
      if ((arp.operation == htons(ARPOP_REPLY)) &&
          (0 == bcmp(arp.tHaddr, mac, 6)) &&
          (0 == bcmp(arp.sInaddr, (uint8_t *)&yiaddr, 4))) {
        LOG(ERROR) << "Valid arp reply receved for this address.";
        rv = 0;
        break;
      }
    }
    timeout -= ::time(NULL) - prevTime;
    ::time(&prevTime);
  }
  CloseSocket(s);
  return rv;
}

}  // namespace MSF