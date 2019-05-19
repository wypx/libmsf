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
#include <msf_network.h>
#include <msf_log.h>
#include <netinet/ip_icmp.h>

#define MSF_MOD_PING "PING"
#define MSF_PING_LOG(level, ...) \
    msf_log_write(level, MSF_MOD_PING, MSF_FUNC_FILE_LINE, __VA_ARGS__)

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
    PINGINTERVAL = 1        /* second */
};

/* common routines */
static s32 in_cksum(u16 *buf, u32 sz)
{
    u32 nleft = sz;
    u32 sum = 0;
    u16 *w = buf;
    u16 ans = 0;

    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1) {
        *(u8 *) (&ans) = *(u8 *) w;
        sum += ans;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    ans = ~sum;
    return (ans);
}


s32 msf_ping(const s8 *host) {

    if (!host) return -1;

    s32 rc = -1;
    struct sockaddr pingaddr;
    struct sockaddr_in *sin = NULL;
    struct sockaddr_in6 *sin6 = NULL;
    struct icmp *pkt;
    s32 pingsock, c = -1;
    s8 packet[DEFDATALEN + MAXIPLEN + MAXICMPLEN];

    memset(&pingaddr, 0, sizeof(struct sockaddr));

    rc = msf_get_sockaddr_by_host(host, &pingaddr);
    if(rc < 0) {
        return -1;
    }

    pingsock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(pingsock < 0) {
        MSF_PING_LOG(DBG_ERROR, "socket error.");
        return -1;
    }

    pkt = (struct icmp *) packet;
    memset(pkt, 0, sizeof(packet));
    pkt->icmp_type = ICMP_ECHO;
    pkt->icmp_cksum = in_cksum((u16 *) pkt, sizeof(packet));

    if (rc == AF_INET) {
        sin = SIN(&pingaddr);
        sin->sin_family = AF_INET ;
        c = sendto(pingsock, packet, DEFDATALEN + ICMP_MINLEN, 0,
            (struct sockaddr *)sin, sizeof(struct sockaddr_in));

    } else if (rc == AF_INET6) {
        sin6 = SIN6(&pingaddr);
        sin6->sin6_family = AF_INET6;
        c = sendto(pingsock, packet, DEFDATALEN + ICMP_MINLEN, 0,
            (struct sockaddr *)sin6, sizeof(struct sockaddr_in6));
    }

    if (c < 0) {
        sclose(pingsock);
        MSF_PING_LOG(DBG_ERROR, "sendto error.");
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
                  (struct sockaddr *) &from, &fromlen)) < 0) {
            if(EAGAIN == errno) {
                rc = -1;
            }
            break;
        }

        if (c >= 76) {/* ip + icmp 76????*/
            struct iphdr *iphdr = (struct iphdr *) packet;

            pkt = (struct icmp *) (packet + (iphdr->ihl << 2)); /* skip ip hdr */
            if(pkt->icmp_type == ICMP_DEST_UNREACH)
            {
                MSF_PING_LOG(DBG_ERROR, "Ping host unreachable.");
                rc = -1;
            }
            if (pkt->icmp_type == ICMP_ECHOREPLY)
                break;
        }
    }
    sclose(pingsock);
    return rc;
}


