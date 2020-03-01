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
#include <base/Logger.h>
#include <netinet/ip_icmp.h>
#include <sock/InetAddress.h>
#include <unistd.h>

using namespace MSF::BASE;
using namespace MSF::SOCK;
/*
  Ping Function:
  to ensure network is valid
  ping luotang.me and dns addr*/

enum PingCode {
  DEFDATALEN = 56,
  MAXIPLEN = 60,
  MAXICMPLEN = 76,
  MAXPACKET = 65468,
  MAX_DUP_CHK = (8 * 128),
  MAXWAIT = 10,
  PINGINTERVAL = 1 /* second */
};

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

int msf_ping(const char *host) {
  if (!host) return -1;

  int rc = -1;
  struct sockaddr pingaddr;
  struct sockaddr_in *sin = NULL;
  struct sockaddr_in6 *sin6 = NULL;
  struct icmp *pkt;
  int pingsock, c = -1;
  char packet[DEFDATALEN + MAXIPLEN + MAXICMPLEN];

  memset(&pingaddr, 0, sizeof(struct sockaddr));

  // rc = msf_get_sockaddr_by_host(host, &pingaddr);
  if (rc < 0) {
    return -1;
  }

  pingsock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (pingsock < 0) {
    // MSF_PING_LOG(DBG_ERROR, "socket error.");
    return -1;
  }

  pkt = (struct icmp *)packet;
  memset(pkt, 0, sizeof(packet));
  pkt->icmp_type = ICMP_ECHO;
  pkt->icmp_cksum = in_cksum((uint16_t *)pkt, sizeof(packet));

  if (rc == AF_INET) {
    sin = SIN(&pingaddr);
    sin->sin_family = AF_INET;
    c = sendto(pingsock, packet, DEFDATALEN + ICMP_MINLEN, 0,
               (struct sockaddr *)sin, sizeof(struct sockaddr_in));

  } else if (rc == AF_INET6) {
    sin6 = SIN6(&pingaddr);
    sin6->sin6_family = AF_INET6;
    c = sendto(pingsock, packet, DEFDATALEN + ICMP_MINLEN, 0,
               (struct sockaddr *)sin6, sizeof(struct sockaddr_in6));
  }

  if (c < 0) {
    close(pingsock);
    // MSF_PING_LOG(DBG_ERROR, "sendto error.");
    return -1;
  }

  struct timeval tv_out;
  tv_out.tv_sec = 2;
  tv_out.tv_usec = 0;
  setsockopt(pingsock, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));

  /* listen for replies */
  while (1) {
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    if ((c = recvfrom(pingsock, packet, sizeof(packet), 0,
                      (struct sockaddr *)&from, &fromlen)) < 0) {
      if (EAGAIN == errno) {
        rc = -1;
      }
      break;
    }

    if (c >= 76) { /* ip + icmp 76????*/
      struct iphdr *iphdr = (struct iphdr *)packet;

      pkt = (struct icmp *)(packet + (iphdr->ihl << 2)); /* skip ip hdr */
      if (pkt->icmp_type == ICMP_DEST_UNREACH) {
        MSF_ERROR << "Ping host unreachable.";
        rc = -1;
      }
      if (pkt->icmp_type == ICMP_ECHOREPLY) break;
    }
  }
  close(pingsock);
  return rc;
}

#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>

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

int msf_arpping(uint32_t yiaddr, uint32_t ip, char *mac, char *interface) {
  int timeout = 2;
  int optval = 1;
  int s = -1;           /* socket */
  int rv = 1;           /* return value */
  struct sockaddr addr; /* for interface name */
  struct arpMsg arp;
  fd_set fdset;
  struct timeval tm;
  time_t prevTime;

  if (-1 == (s = socket(PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP)))) {
    MSF_ERROR << "Could not create raw socket.";
    return -1;
  }

  if (-1 == setsockopt(s, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval))) {
    MSF_ERROR << "Could not setsocketopt on raw socket.";
    close(s);
    return -1;
  }

  /*http://blog.csdn.net/wanxiao009/archive/2010/05/21/5613581.aspx */
  memset(&arp, 0, sizeof(arp));
  memcpy(arp.ethhdr.h_dest, MAC_BCAST_ADDR, 6); /* MAC DA */
  memcpy(arp.ethhdr.h_source, mac, 6);          /* MAC SA */
  arp.ethhdr.h_proto = htons(ETH_P_ARP);        /* protocol type (Ethernet) */
  arp.htype = htons(ARPHRD_ETHER);              /* hardware type */
  arp.ptype = htons(ETH_P_IP);                /* protocol type (ARP message) */
  arp.hlen = 6;                               /* hardware address length */
  arp.plen = 4;                               /* protocol address length */
  arp.operation = htons(ARPOP_REQUEST);       /* ARP op code */
  memcpy(arp.sInaddr, (uint8_t *)&ip, 4);     /* source IP address */
  memcpy(arp.sHaddr, mac, 6);                 /* source hardware address */
  memcpy(arp.tInaddr, (uint8_t *)&yiaddr, 4); /* target IP address */

  memset(&addr, 0, sizeof(addr));
  strcpy(addr.sa_data, interface);
  if (sendto(s, &arp, sizeof(arp), 0, &addr, (socklen_t)sizeof(addr)) < 0)
    rv = 0;

  tm.tv_usec = 0;
  time(&prevTime);
  while (timeout > 0) {
    FD_ZERO(&fdset);
    FD_SET(s, &fdset);
    tm.tv_sec = timeout;
    if (select(s + 1, &fdset, (fd_set *)NULL, (fd_set *)NULL, &tm) < 0) {
      MSF_ERROR << "Error on ARPING request: " << strerror(errno);
      if (errno != EINTR) rv = 0;
    } else if (FD_ISSET(s, &fdset)) {
      if (recv(s, &arp, sizeof(arp), 0) < 0) rv = 0;
      if ((arp.operation == htons(ARPOP_REPLY)) &&
          (0 == bcmp(arp.tHaddr, mac, 6)) &&
          (0 == bcmp(arp.sInaddr, (uint8_t *)&yiaddr, 4))) {
        MSF_ERROR << "Valid arp reply receved for this address.";
        rv = 0;
        break;
      }
    }
    timeout -= time(NULL) - prevTime;
    time(&prevTime);
  }
  close(s);
  return rv;
}
