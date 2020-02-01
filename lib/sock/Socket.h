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
#ifndef __MSF_SOCKET_H__
#define __MSF_SOCKET_H__

#include <base/Noncopyable.h>
#include <sock/InetAddress.h>
#include <tuple>

#include <stdio.h>  // snprintf
#include <unistd.h>
#include <iostream>
#include <fcntl.h>

#include <netinet/tcp.h>

using namespace MSF::BASE;
using namespace MSF::SOCK;

namespace MSF {
namespace SOCK {

#ifdef WIN32
#include <winsock2.h>
#define SOCKET_ERRNO(error) WSA##error
#define socket_close closesocket
#define socket_errno WSAGetLastError()
#define socket_strerror gai_strerrorA//gai_strerror
#endif

/* Needed on Mac OS/X */
#ifndef SOL_TCP
#define SOL_TCP IPPROTO_TCP
#endif

#define IS_TCP(x)       (x == SOCK_STREAM || x == SOCK_STREAM)
#define IS_UDP(x)       (x == SOCK_DGRAM || x == SOCK_DGRAM)
#define IS_UNIX(x)      (x == SOCK_STREAM)
#define IS_MUTICAST(x)  (x == SOCK_STREAM)

int CreateTCPSocket();
int CreateUDPSocket();
int CreateUnixSocket();
int MakePipe(int fd[2], bool isBlocking);
bool CreateSocketPair(int fd[2]);
void CloseSocket(const int fd);
void ShutdownWrite(const int fd);
void DebugSocket(const int fd);

bool SetNonBlock(const int fd, bool on = true);
bool SetSndBuf(const int fd, const uint32_t size = 64 * 1024);
bool SetRcvBuf(const int fd, const uint32_t size = 64 * 1024);
bool SetSegBufferSize(const int fd, const uint32_t size);
bool SetSendBufferSize(const int fd, const uint32_t size);
/* Enable/disable SO_REUSEADDR */
bool SetReuseAddr(const int fd, bool on = true);
/* Enable/disable SO_REUSEPORT */
bool SetReusePort(const int fd, bool on = true);
bool SetCloseOnExec(const int fd);
bool SetDcsp(const int fd, const uint32_t family, const uint32_t dscp);
bool SetTimeout(const uint32_t timeotMs);

bool SetIpv6Only(const int fd, bool on);
/************************* TCP SOCKET FEATURES ******************************/

#if defined (__linux__)
// Sometimes these flags are not present in the headers,
// so define them if not present.
#if !defined(MSG_FASTOPEN)
#define MSG_FASTOPEN 0x20000000
#endif

#if !defined(TCP_FASTOPEN)
#define TCP_FASTOPEN 23
#endif

#if !defined(TCPI_OPT_SYN_DATA)
#define TCPI_OPT_SYN_DATA 32
#endif
#endif

int CreateTcpServer(const std::string &host, 
                const uint16_t port,
                const uint32_t proto,
                const uint32_t backlog);

bool Connect(const int fd, const struct sockaddr *srvAddr,
            socklen_t addrLen, const int timeout);
int ConnectTcpServer(const std::string &host, 
                const uint16_t port,
                const uint32_t proto);
/* Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).*/
bool SetTcpNoDelay(const int fd, bool on = true);
/* Enable/disable TCP_CORK */
bool SetTcpNoPush(const int fd, bool on = true);

bool SetTcpFastOpen(const int fd);
bool SetTcpDeferAccept(const int fd);

/* Enable/disable SO_KEEPALIVE */
bool SetKeepAlive(const int fd, bool on);

bool SetLinger(const int fd, bool on);

bool GetTcpInfo(const int fd, struct tcp_info*);
bool GetTcpInfoString(const int fd, char* buf, int len);


int SendMsg(const int fd, struct iovec *iov, int cnt, int flag);
int RecvMsg(const int fd, struct iovec *iov, int cnt, int flag);
int tfoSendMsg(const int fd, struct iovec *iov, int cnt, int flag);

/************************* TCP SOCKET FEATURES ******************************/


/************************* UDP SOCKET FEATURES ******************************/
int SendTo(const int fd, const char *buf, const int len);
int RecvFrom(const int fd, char *buf, const int len);
/************************* UDP SOCKET FEATURES ******************************/

/************************* MUTICAST SOCKET FEATURES ******************************/
#define DEFAULT_MUTICAST_ADDR "224.1.2.3"
int CreateMuticastFd();
/************************* MUTICAST SOCKET FEATURES ******************************/

/************************* UNIX SOCKET FEATURES ******************************/
int CreateUnixServer(const std::string & srvPath, const uint32_t backlog, const uint32_t access_mask);
int ConnectUnixServer(const std::string & srvPath, const std::string & cliPath);
/************************* UNIX SOCKET FEATURES ******************************/

/************************* EVENT SOCKET FEATURES ******************************/
int CreateEventFd(const int initval, const int flags);
bool NotifyEvent();
bool ClearEvent();
/************************* EVENT SOCKET FEATURES ******************************/

/************************* TIMER SOCKET FEATURES ******************************/
/*
struct itimerspec 
{
    struct timespec it_interval;   //间隔时间
    struct timespec it_value;      //第一次到期时间
};
struct timespec 
{
    time_t tv_sec;    //秒
    long tv_nsec;    //纳秒
}; 
*/
int CreateTimerFd();
bool UpdateTimerFd(const int fd);
/************************* TIMER SOCKET FEATURES ******************************/

/************************* SIGNAL SOCKET FEATURES ******************************/
int CreateSigalFd(void);
/************************* SIGNAL SOCKET FEATURES ******************************/

/************************* NETLINK SOCKET FEATURES ******************************/
//https://blog.csdn.net/kanda2012/article/details/7580623
//https://www.cnblogs.com/wenqiang/p/6306727.html


/************************* NETLINK SOCKET FEATURES ******************************/


void DrianData(const int fd, const uint32_t size);

uint64_t GetMaxOpenFd();
bool SetMaxOpenFd(uint64_t maxfds);


// namespace std {
// template<>
// struct hash<ananas::SocketAddr> {
//     typedef ananas::SocketAddr argument_type;
//     typedef std::size_t result_type;
//     result_type operator()(const argument_type& s) const noexcept {
//         result_type h1 = std::hash<short> {}(s.GetAddr().sin_family);
//         result_type h2 = std::hash<unsigned short> {}(s.GetAddr().sin_port);
//         result_type h3 = std::hash<unsigned int> {}(s.GetAddr().sin_addr.s_addr);
//         result_type tmp = h1 ^ (h2 << 1);
//         return h3 ^ (tmp << 1);
//     }
// };
// }

}
}
#endif