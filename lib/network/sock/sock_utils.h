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
#ifndef SOCK_SOCK_UITILS_H_
#define SOCK_SOCK_UITILS_H_

#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <signal.h>
#include <sys/uio.h>
#include <sys/epoll.h>
#include <string>

#define HAVE_MSGHDR_MSG_CONTROL 1

struct sockaddr;
struct sockaddr_in;

namespace MSF {

// Differentiated Services Code Point.
// See http://tools.ietf.org/html/rfc2474 for details.
enum DiffServCodePoint {
  DSCP_NO_CHANGE = -1,
  DSCP_DEFAULT = 0,  // Same as DSCP_CS0
  DSCP_CS0 = 0,      // The default
  DSCP_CS1 = 8,      // Bulk/background traffic
  DSCP_AF11 = 10,
  DSCP_AF12 = 12,
  DSCP_AF13 = 14,
  DSCP_CS2 = 16,
  DSCP_AF21 = 18,
  DSCP_AF22 = 20,
  DSCP_AF23 = 22,
  DSCP_CS3 = 24,
  DSCP_AF31 = 26,
  DSCP_AF32 = 28,
  DSCP_AF33 = 30,
  DSCP_CS4 = 32,
  DSCP_AF41 = 34,  // Video
  DSCP_AF42 = 36,  // Video
  DSCP_AF43 = 38,  // Video
  DSCP_CS5 = 40,   // Video
  DSCP_EF = 46,    // Voice
  DSCP_CS6 = 48,   // Voice
  DSCP_CS7 = 56,   // Control messages
};

static const int kInvalidSocket = -1;

#ifdef WIN32
#include <winsock2.h>
#define SOCKET_ERRNO(error) WSA##error
#define socket_close closesocket
#define socket_errno WSAGetLastError()
#define socket_strerror gai_strerrorA  // gai_strerror
#endif

bool SocketInit();

bool SocketError(const int fd);
bool IsNoNetwork(const int error);
bool SocketIsConnected(const int fd);
bool SocketIsListener(const int fd);
bool IsTcpConnected(const int fd);

void DebugSocket(const int fd);
int CreateSocket(const int domain, const int type, const int protocol);
int CreateTCPSocket();
int CreateUDPSocket();
int CreateICMPSocket();
int CreateUnixSocket();
int CreatePipe(int fd[2], bool is_blocking);
bool CreateSocketPair(int fd[2]);
void CloseSocket(const int fd);
void Shutdown(const int fd, int how);
bool SetIpv6Only(const int fd, bool on);
bool SetTimeout(const int fd, const int timeoutMs);
bool SetNonBlock(const int fd, bool on);
bool SetCloseOnExec(const int fd);
int OpenDevNull(bool rw = true);
bool SetTcpNoDelay(const int fd, bool on);
bool SetReuseAddr(const int fd, bool on);
bool SetReusePort(const int fd, bool on);
bool SetKeepAlive(const int fd, bool on);
bool SetLinger(const int fd, bool on);
bool SetSegBufferSize(const int fd, const uint32_t n);
bool SetSendBufferSize(const int fd, const int n);

bool BindDevice(const int fd, const char *ifname);
bool BindAddress(const int fd, const struct sockaddr *addr,
                 const socklen_t addrlen);

int CreateTcpServer(const std::string &host, const uint16_t port,
                    const uint32_t proto, const uint32_t backlog);
int CreateUnixServer(const std::string &srvPath, const uint32_t backlog,
                     const uint32_t access_mask);
int ConnectTcpServer(const std::string &host, const uint16_t port,
                     const uint32_t proto);
int ConnectUnixServer(const std::string &srvPath, const std::string &cliPath);

int ReadFdMessage(int sockfd, char *buf, int buflen, int *fds, int fd_num);
int SendFdMessage(int sockfd, char *buf, int buflen, int *fds, int fd_num);

int GetRAWSocketTTL(const int fd, const int family);
int SetRAWSocketTTL(const int fd, const int family, int ttl);
int GetRAWSocketTOS(const int fd, const int family);
int SetRAWSocketTOS(const int fd, const int family, int tos);
bool SetDCSP(const int fd, const uint32_t family, const uint32_t dscp);
int GetSocketMTU(const int fd, int family);
int SetSocketDF(const int fd, const int family, const int value);

void DrianData(const int fd, const uint32_t size);
int SendMsg(const int fd, struct iovec *iov, int cnt, int flag);
int RecvMsg(const int fd, struct iovec *iov, int cnt, int flag);
int SendTo(const int fd, const char *buf, const int len);
int RecvFrom(const int fd, char *buf, const int len);

int SocketWriteQueue(const int fd, int *write_q);
int SocketReadQueue(const int fd, int *read_q);
int64_t GetSocketRecvTimestamp(int fd);

int TFOSendMsg(const int fd, struct iovec *iov, int cnt, int flag);
bool SetTcpNoPush(const int fd, bool on);
bool SetTcpFastOpen(const int fd);
bool SetTcpDeferAccept(const int fd);
bool GetTcpInfoString(const int fd, char *buf, int len);
int SetTcpMSS(const int fd, const int size);
int GetTcpMSS(const int fd, int *size);

bool SetMuticalastTTL(const int fd, const int ttl);
bool SetMuticalastIF(const int fd, const std::string &iface);
bool SetMuticalastLoop(const int fd, uint32_t af, const int loop);
bool AddMuticalastMemershipv4(const int fd, const std::string &ip);
bool AddMuticalastMemershipv6(const int fd, const std::string &ip);
bool DropMuticalastMemershipv4(const int fd, const std::string &ip);
bool DropMuticalastMemershipv6(const int fd, const std::string &ip);
int ReadMuticastData(const int fd, struct sockaddr_in *addr);
int CreateMuticastFd(const std::string &ip, const uint16_t port);

int CreateEventFd(const int initval, const int flags);
bool NotifyEvent(const int fd);
bool ClearEvent(const int fd);

int CreateTimerFd();
bool UpdateTimerFd(const int fd);

int CreateSigalFd(void);

uint64_t GetMaxOpenFd();
bool SetMaxOpenFd(uint64_t maxfds);
}  // namespace MSF

#endif