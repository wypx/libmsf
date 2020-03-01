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
#include <arpa/inet.h>
#include <base/Logger.h>
#include <base/Utils.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <sock/Socket.h>
#include <sys/eventfd.h>
#include <sys/poll.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/stat.h> /* umask chmod*/
#include <sys/timerfd.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* netlink socket */
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/socket.h>
#include <netinet/ip.h>

namespace MSF {
namespace SOCK {

/*
 * The first epoll version has been introduced in Linux 2.5.44.  The
 * interface was changed several times since then and the final version
 * of epoll_create(), epoll_ctl(), epoll_wait(), and EPOLLET mode has
 * been introduced in Linux 2.6.0 and is supported since glibc 2.3.2.
 *
 * EPOLLET mode did not work reliable in early implementaions and in
 * Linux 2.4 backport.
 *
 * EPOLLONESHOT             Linux 2.6.2,  glibc 2.3.
 * EPOLLRDHUP               Linux 2.6.17, glibc 2.8.
 * epoll_pwait()            Linux 2.6.19, glibc 2.6.
 * signalfd()               Linux 2.6.22, glibc 2.7.
 * eventfd()                Linux 2.6.22, glibc 2.7.
 * timerfd_create()         Linux 2.6.25, glibc 2.8.
 * epoll_create1()          Linux 2.6.27, glibc 2.9.
 * signalfd4()              Linux 2.6.27, glibc 2.9.
 * eventfd2()               Linux 2.6.27, glibc 2.9.
 * accept4()                Linux 2.6.28, glibc 2.10.
 * eventfd2(EFD_SEMAPHORE)  Linux 2.6.30, glibc 2.10.
 * EPOLLEXCLUSIVE           Linux 4.5, glibc 2.24.
 */

// static uint32_t g_oldGateway = 0;

bool SocketInit() {
  char *errstr;
#ifdef WIN32
  WORD wVersionRequested;
  WSADATA wsaData;
  int error_code;

  wVersionRequested = MAKEWORD(2, 0);
  if ((error_code = WSAStartup(wVersionRequested, &wsaData)) != 0) {
    *errstr = xasprintf("%s", wsa_strerror(error_code));
    return false;
  } else {
    return false;
  }
#else /* noone else needs this... */
  (void)errstr;
  return true;
#endif
}

int CreateSocket(const int domain, const int type, const int protocol) {
  int fd;
#ifdef SOCK_CLOEXEC
  fd = socket(domain, type | SOCK_CLOEXEC, protocol);
  if (fd == -1) {
    MSF_ERROR << "Failed to open socket:" << strerror(errno);
    return fd;
  }
#else
  fd = socket(domain, type, protocol);
  if (fd == -1) {
    MSF_ERROR << "Failed to open socket:" << strerror(errno);
    return fd;
  }
  SetCloseOnExec(fd);
#endif
  return fd;
}

int CreateTCPSocket() {
  return CreateSocket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}
int CreateUDPSocket() { return CreateSocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); }
int CreateUnixSocket() { return CreateSocket(AF_UNIX, SOCK_STREAM, 0); }

bool CreateSocketPair(int fd[2]) {
  // pipe
  int ret = socketpair(AF_LOCAL, SOCK_STREAM, IPPROTO_TCP, fd);
  if (ret != 0) {
    return false;
  }
  return true;
}

/* http://man7.org/linux/man-pages/man2/pipe2.2.html */
// #define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <fcntl.h> /* Obtain O_* constant definitions */
#include <unistd.h>

int MakePipe(int fd[2], bool is_blocking) {
  return pipe2(fd, is_blocking ? 0 : O_NONBLOCK);
  // return pipe(fd);
}

void CloseSocket(const int fd) {
  if (fd > 0) {
#ifdef WIN32
    (void)closesocket(fd);
#else
    close(fd);
#endif
  }
}

/*
1.close()函数
    #include<unistd.h>
    int close(int sockfd); //返回成功为0,出错为-1.</SPAN>
    #include<unistd.h>
    int close(int sockfd); //返回成功为0，出错为-1.
    close 一个套接字的默认行为是把套接字标记为已关闭,然后立即返回到调用进程,
    该套接字描述符不能再由调用进程使用,
    也就是说它不能再作为read或write的第一个参数,
    然而TCP将尝试发送已排队等待发送到对端的任何数据，
    发送完毕后发生的是正常的TCP连接终止序列.
    在多进程并发服务器中，父子进程共享着套接,
    套接字描述符引用计数记录着共享着的进程个数,
    当父进程或某一子进程close掉套接字时，
    描述符引用计数会相应的减一，当引用计数仍大于零时,
    这个close调用就不会引发TCP的四路握手断连过程。
2.shutdown()函数
    #include<sys/socket.h>
    int shutdown(int sockfd,int howto);  //返回成功为0，出错为-1.</SPAN>
    #include<sys/socket.h>
    int shutdown(int sockfd,int howto);  //返回成功为0，出错为-1.
该函数的行为依赖于howto的值 1.SHUT_RD：值为0，关闭连接的读这一半。
    2.SHUT_WR：值为1，关闭连接的写这一半。
    3.SHUT_RDWR：值为2，连接的读和写都关闭。
    终止网络连接的通用方法是调用close函数
    但使用shutdown能更好的控制断连过程（使用第二个参数）。
3.两函数的区别
    close与shutdown的区别主要表现在：
    close函数会关闭套接字ID，如果有其他的进程共享着这个套接字,
    那么它仍然是打开的, 这个连接仍然可以用来读和写,
    并且有时候这是非常重要的, 特别是对于多进程并发服务器来说.
    而shutdown会切断进程共享的套接字的所有连接,
    不管这个套接字的引用计数是否为零,那些试图读得进程将会接收到EOF标识,
    那些试图写的进程将会检测到SIGPIPE信号,同时可利用shutdown的第二个参数选择断连的方式.
*/

void ShutdownWrite(const int fd) {}

bool SocketIsConnected(const int fd) {
  struct sockaddr junk;
  socklen_t length = sizeof(junk);
  memset(&junk, 0, sizeof(junk));
  return getpeername(fd, &junk, &length) == 0;
}

// https://www.cnblogs.com/tinaluo/p/7792152.html
bool SocketIsListener(const int fd) {
  int val;
  socklen_t len = sizeof(val);
  if (getsockopt(fd, SOL_SOCKET, SO_ACCEPTCONN, &val, &len) < 0) {
    return false;
  }
  return true;
}

void DebugSocket(const int fd) {
  int ret;
  char ip[64];
  struct sockaddr_in *sin = NULL;
  struct sockaddr_in6 *sin6 = NULL;
  struct sockaddr_un *cun = NULL;
  struct sockaddr_storage cliAddr;
  socklen_t addrlen = sizeof(cliAddr);

  memset(ip, 0, sizeof(ip));
  memset(&cliAddr, 0, sizeof(cliAddr));

  ret = getsockname(fd, (struct sockaddr *)&cliAddr, &addrlen);
  if (ret < 0) {
    MSF_ERROR << "Conn getsockname failed,";
    return;
  }
  if (cliAddr.ss_family == AF_UNIX) {
    cun = SINU(&cliAddr);
    MSF_INFO << "new conn from unix path: " << cun->sun_path;
  } else if (cliAddr.ss_family == AF_INET) {
    sin = SIN(&cliAddr);
    /* inet_ntoa(sin->sin_addr) */
    inet_ntop(cliAddr.ss_family, &sin->sin_addr, ip, sizeof(ip));
    MSF_INFO << "new conn from ipv4: " << ip
             << ", port: " << ntohs(sin->sin_port);
  } else if (cliAddr.ss_family == AF_INET6) {
    sin6 = SIN6(&cliAddr);
    inet_ntop(cliAddr.ss_family, &sin6->sin6_addr, ip, sizeof(ip));
    MSF_INFO << "new conn from ipv6: " << ip
             << ", port: " << ntohs(sin6->sin6_port);
  }

  memset(ip, 0, sizeof(ip));
  memset(&cliAddr, 0, sizeof(cliAddr));
  ret = getpeername(fd, (struct sockaddr *)&cliAddr, &addrlen);
  if (ret < 0) {
    // MSF_ERROR << "Conn getpeername failed";
    return;
  }

  if (cliAddr.ss_family == AF_UNIX) {
    cun = SINU(&cliAddr);
    MSF_INFO << "new conn from unix path: " << cun->sun_path;
  } else if (cliAddr.ss_family == AF_INET) {
    sin = SIN(&cliAddr);
    /* inet_ntoa(sin->sin_addr) */
    inet_ntop(cliAddr.ss_family, &sin->sin_addr, ip, sizeof(ip));
    MSF_INFO << "new conn from ipv4: " << ip
             << ", port: " << ntohs(sin->sin_port);
  } else if (cliAddr.ss_family == AF_INET6) {
    sin6 = SIN6(&cliAddr);
    inet_ntop(cliAddr.ss_family, &sin6->sin6_addr, ip, sizeof(ip));
    MSF_INFO << "new conn from ipv6: " << ip
             << ", port: " << ntohs(sin6->sin6_port);
  }
}

bool SetReuseAddr(const int fd, bool on) {
#if defined(SO_REUSEADDR) && !defined(_WIN32)
  int one = on;
  /* REUSEADDR on Unix means, "don't hang on to this address after the
   * listener is closed."  On Windows, though, it means "don't keep other
   * processes from binding to this address while we're using it. */
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&one, (int)sizeof(one)) <
      0) {
    return false;
  }
#endif
  return true;
}

bool SetReusePort(const int fd, bool on) {
#if defined __linux__ && defined(SO_REUSEPORT)
  int one = on;
  /* REUSEPORT on Linux 3.9+ means, "Multiple servers (processes or
   * threads) can bind to the same port if they each set the option. */
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &one, (int)sizeof(one)) < 0) {
    return false;
  }
#endif
  return true;
}

/* http://blog.csdn.net/yangzhao0001/article/details/48003337
 * Set the socket send timeout (SO_SNDTIMEO socket option) to the specified
 * number of milliseconds, or disable it if the 'ms' argument is zero. */
bool SetTimeout(const int fd, const int timeoutMs) {
  if (timeoutMs <= 0) {
    return true;
  }

#ifdef WIN32
  DWORD milliseconds = timeoutMs * 1000;
  if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &milliseconds, sizeof(int)) <
      0) {
    return false;
  }
  if (setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &milliseconds, sizeof(int)) <
      0) {
    return false;
  }
  return true;
#else /* Linux or DJGPP */
  struct timeval tv;
  tv.tv_sec = timeoutMs / 1000;
  tv.tv_usec = (timeoutMs % 1000) * 1000;

  if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv) < 0)) {
    MSF_ERROR << "Fail to set send timeout for socket: " << fd;
    return false;
  }
  if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv) < 0)) {
    MSF_ERROR << "Fail to set recv timeout for socket: " << fd;
    return false;
  }
  return true;
#endif
}
/* ioctl(FIONBIO) sets a non-blocking mode with the single syscall
 * while fcntl(F_SETFL, O_NONBLOCK) needs to learn the current state
 * using fcntl(F_GETFL).
 *
 * ioctl() and fcntl() are syscalls at least in FreeBSD 2.x, Linux 2.2
 * and Solaris 7.
 *
 * ioctl() in Linux 2.4 and 2.6 uses BKL, however, fcntl(F_SETFL) uses it too.*/
bool SetNonBlock(const int fd, bool on) {
#ifdef WIN32
  int nonblocking = on;
  if (ioctlsocket(fd, FIONBIO, &nonblocking) == -1) {
    MSF_ERROR << "Fail to set nonblocking for socket: " << fd;
    return false;
  }
#else
  int flags;
  if ((flags = fcntl(fd, F_GETFL, NULL)) < 0) {
    MSF_ERROR << "Fail to get nonblocking flag for socket: " << fd;
    return false;
  }
  if (!(flags & O_NONBLOCK)) {
    if (on == true) {
      if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        MSF_ERROR << "Fail to set nonblocking for socket: " << fd;
        return false;
      }
      SetCloseOnExec(fd);
    }
  } else {
    if (on == false) {
      if (fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) < 0) {
        MSF_ERROR << "Fail to set blocking for socket: " << fd;
        return false;
      }
      SetCloseOnExec(fd);
    }
  }
#endif
  return true;
}

/* Do platform-specific operations as needed to close a socket upon a
 * successful execution of one of the exec*() functions.*/
bool SetCloseOnExec(const int fd) {
#if !defined(_WIN32)
  int flags;
  if ((flags = fcntl(fd, F_GETFD, NULL)) < 0) {
    return false;
  }
  if (!(flags & FD_CLOEXEC)) {
    if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1) {
      return false;
    }
  }
#endif
  return true;
}

bool SetSegBufferSize(const int fd, const uint32_t n) {
#if 0
    int size = 1024;
	if (setsockopt(fd, IPPROTO_TCP, TCP_MAXSEG, &size, sizeof(int)) < 0) {

    }
#endif
  return true;
}

/*
 * Sets a socket's send buffer size to the maximum allowed by the system.
 */
bool SetSendBufferSize(const int fd, const uint32_t n) {
#define MAX_SENDBUF_SIZE (256 * 1024 * 1024)  // 16 * 1024
  socklen_t intsize = sizeof(int);
  int min, max, avg;
  int old_size;

  /* Start with the default size. */
  if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &old_size, &intsize) != 0) {
    perror("getsockopt(SO_SNDBUF)");
    return false;
  }

  /* Binary-search for the real maximum. */
  min = old_size;
  max = MAX_SENDBUF_SIZE;

  while (min <= max) {
    avg = ((uint32_t)(min + max)) / 2;
    /*
     * On Unix domain sockets
     *   Linux uses 224K on both send and receive directions;
     *   FreeBSD, MacOSX, NetBSD, and OpenBSD use 2K buffer size
     *   on send direction and 4K buffer size on receive direction;
     *   Solaris uses 16K on send direction and 5K on receive direction.
     */
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void *)&avg, intsize) == 0) {
      min = avg + 1;
    } else {
      max = avg - 1;
    }
  }
  // MSF_INFO << "send buffer old: " << old_size << " new: " << avg;
  return true;
}

/* Sets the DSCP value of socket 'fd' to 'dscp', which must be 63 or less.
 * 'family' must indicate the socket's address family (AF_INET or AF_INET6, to
 * do anything useful). */
bool SetDcsp(const int fd, const uint32_t family, const uint32_t dscp) {
  int retval;
  int val;

#ifdef WIN32
  /* XXX: Consider using QoS2 APIs for Windows to set dscp. */
  return 0;
#endif

  if (dscp > 63) {
    MSF_WARN << "Qos dscp muts less than 63";
    return false;
  }
  val = dscp << 2;
  switch (family) {
    case AF_INET:
      retval = setsockopt(fd, IPPROTO_IP, IP_TOS, &val, sizeof val);
      break;
    case AF_INET6:
      retval = setsockopt(fd, IPPROTO_IPV6, IPV6_TCLASS, &val, sizeof val);
      break;
    default:
      errno = ENOPROTOOPT;
      return false;
  }
  return retval ? false : true;
}

/* Reads and discards up to 'n' datagrams from 'fd', stopping as soon as no
 * more data can be immediately read.	('fd' should therefore be in
 * non-blocking mode.)*/
void DrianData(const int fd, const uint32_t size) {
  uint32_t n = size;
  char buffer[128];
  for (; n > 0; n--) {
    /* 'buffer' only needs to be 1 byte long in most circumstances.  This
     * size is defensive against the possibility that we someday want to
     * use a Linux tap device without TUN_NO_PI, in which case a buffer
     * smaller than sizeof(struct tun_pi) will give EINVAL on read. */
    if (read(fd, buffer, sizeof buffer) <= 0) {
      break;
    }
  }
}

int SendMsg(const int fd, struct iovec *iov, int cnt, int flag) {
  struct msghdr msg = {0};

  msg.msg_iov = iov;
  msg.msg_iovlen = cnt;
  return sendmsg(fd, &msg, flag);
}

int RecvMsg(const int fd, struct iovec *iov, int cnt, int flag) {
  struct msghdr msg = {0};

  msg.msg_iov = iov;
  msg.msg_iovlen = cnt;
  return recvmsg(fd, &msg, flag);
}

bool SocketError(const int fd) {
  int error = -1;
  socklen_t len = sizeof(error);
  if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
    MSF_ERROR << "Socket get errno: " << errno << " error: " << error;
    return true;
  }
  return false;
}

bool IsNoNetwork(int error) {
  if (error == ENETDOWN || error == ENETUNREACH) return true;
  return false;
}

int socket_get_nwrite(const int fd, int *_nwriteLen) {
#if defined(__APPLE__)
  socklen_t len = sizeof(int);
  return getsockopt(fd, SOL_SOCKET, SO_NWRITE, _nwriteLen, &len);
#elif defined(ANDROID)
  return ioctl(fd, SIOCOUTQ, _nwriteLen);
#else
  *_nwriteLen = -1;
  return 0;
#endif
}

int socket_get_nread(const int fd, int *_nreadLen) {
#if defined(__APPLE__)
  socklen_t len = sizeof(int);
  return getsockopt(fd, SOL_SOCKET, SO_NREAD, _nreadLen, &len);
#elif defined(ANDROID)
  return ioctl(fd, SIOCINQ, _nreadLen);
#else
  *_nreadLen = -1;
  return 0;
#endif
}

bool SetIpv6Only(const int fd, bool on) {
#ifdef IPV6_V6ONLY
  int only = on;
  if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&only, sizeof(only)) <
      0) {
    MSF_ERROR << "Fail to set ipv6 only for: " << fd;
    return false;
  }
#endif
  return true;
}
/************************* TCP SOCKET FEATURES ******************************/
/**
 * 参考文章
 * https://www.cnblogs.com/gwwdq/p/9261615.html
 * https://blog.csdn.net/zhangyu627836411/article/details/80215746
 * http://blog.chinaunix.net/uid-29110326-id-4749991.html
 * */
bool GetTcpInfo(const int fd, struct tcp_info *tcpi) {
  socklen_t len = sizeof(*tcpi);
  memset(tcpi, 0, len);
  return ::getsockopt(fd, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
}

bool GetTcpInfoString(const int fd, char *buf, int len) {
  struct tcp_info tcpi;
  bool ok = GetTcpInfo(fd, &tcpi);
  if (ok) {
    snprintf(
        buf, len,
        "unrecovered=%u "
        "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
        "lost=%u retrans=%u rtt=%u rttvar=%u "
        "sshthresh=%u cwnd=%u total_retrans=%u",
        tcpi.tcpi_retransmits,  // Number of unrecovered [RTO] timeouts
        tcpi.tcpi_rto,          // Retransmit timeout in usec
        tcpi.tcpi_ato,          // Predicted tick of soft clock in usec
        tcpi.tcpi_snd_mss, tcpi.tcpi_rcv_mss,
        tcpi.tcpi_lost,     // Lost packets
        tcpi.tcpi_retrans,  // Retransmitted packets out
        tcpi.tcpi_rtt,      // Smoothed round trip time in usec
        tcpi.tcpi_rttvar,   // Medium deviation
        tcpi.tcpi_snd_ssthresh, tcpi.tcpi_snd_cwnd,
        tcpi.tcpi_total_retrans);  // Total retransmits for entire connection
  }
  return ok;
}

// not support in windows
int SetTcpMss(const int fd, int size) {
#ifdef WIN32
  return 0;
#else
  return setsockopt(fd, IPPROTO_TCP, TCP_MAXSEG, &size, sizeof(size));
#endif
}

int GetTcpMss(const int fd, int *size) {
#ifdef WIN32
  return 0;
#else
  if (size == nullptr) return -1;
  socklen_t len = sizeof(int);
  return getsockopt(fd, IPPROTO_TCP, TCP_MAXSEG, (void *)size, &len);
#endif
}

/* 参考https://blog.csdn.net/c_cyoxi/article/details/8673645
 *
 * */
bool SetTcpNoDelay(const int fd, bool on) {
  int opt = 0;
  socklen_t optlen = sizeof(opt);
  if (0 == getsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, &optlen)) {
    if (opt) {
      return false; /* TCP_NODELAY inherited from listen socket */
    }
  }
  opt = 1;
  if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
    MSF_ERROR << "setsockopt TCP_NODELAY: " << strerror(errno);
    return false;
  }
  return true;
}

extern "C" int _SocketTcpNagleOff(int fd) {
  MSF_INFO << "Test alias function.";
  return 0;
}

static int SocketTcpNagleOffv1(int fd)
    MSF_ATTRIBUTE_WIKEREF_ALIAS(SocketTcpNodelay);

int SocketTcpNagleOffv2(int fd) MSF_ATTRIBUTE_WIKE_ALIAS(_SocketTcpNagleOff);

// second argument (target name can't have namespace)
ALIAS_FUNCTION(SocketTcpNagleOff, SocketTcpNodelay);

/**
 * linux tcp_cork,上面的意思就是说,当使用sendfile函数时,
 * tcp_nopush才起作用,它和指令tcp_nodelay是互斥的
 * 这是tcp/ip传输的一个标准了,这个标准的大概的意思是,
 * 一般情况下,在tcp交互的过程中,当应用程序接收到数据包后马上传送出去,不等待,
 * 而tcp_cork选项是数据包不会马上传送出去,等到数据包最大时,一次性的传输出去,
 * 这样有助于解决网络堵塞,已经是默认了.
 * 也就是说tcp_nopush = on 会设置调用tcp_cork方法,这个也是默认的,
 * 结果就是数据包不会马上传送出去,等到数据包最大时,一次性的传输出去,
 * 这样有助于解决网络堵塞
 *
 * 以快递投递举例说明一下（以下是我的理解,也许是不正确的），当快递东西时,
 * 快递员收到一个包裹,马上投递,这样保证了即时性,但是会耗费大量的人力物力,
 * 在网络上表现就是会引起网络堵塞,而当快递收到一个包裹,把包裹放到集散地,
 * 等一定数量后统一投递,这样就是tcp_cork的选项干的事情,这样的话,
 * 会最大化的利用网络资源,虽然有一点点延迟.
 * 这个选项对于www，ftp等大文件很有帮助
 *
 * TCP_NODELAY和TCP_CORK基本上控制了包的“Nagle化”
 * Nagle化在这里的含义是采用Nagle算法把较小的包组装为更大的帧.
 * John Nagle是Nagle算法的发明人,后者就是用他的名字来命名的,
 * 他在1984年首次用这种方法来尝试解决福特汽车公司的网络拥塞问题
 * (欲了解详情请参看IETF RFC 896).
 * 他解决的问题就是所谓的silly window syndrome,
 * 中文称“愚蠢窗口症候群”，具体含义是,
 * 因为普遍终端应用程序每产生一次击键操作就会发送一个包,
 * 而典型情况下一个包会拥有一个字节的数据载荷以及40个字节长的包头,
 * 于是产生4000%的过载,很轻易地就能令网络发生拥塞.
 * Nagle化后来成了一种标准并且立即在因特网上得以实现.
 * 它现在已经成为缺省配置了，但在我们看来,
 * 有些场合下把这一选项关掉也是合乎需要的。
 *
 * 现在让我们假设某个应用程序发出了一个请求,希望发送小块数据.
 * 我们可以选择立即发送数据或者等待产生更多的数据然后再一次发送两种策略.
 * 如果我们马上发送数据,那么交互性的以及客户/服务器型的应用程序将极大地受益.
 * 如果请求立即发出那么响应时间也会快一些.
 * 以上操作可以通过设置套接字的TCP_NODELAY = on 选项来完成,
 * 这样就禁用了Nagle 算法。
 * 另外一种情况则需要我们等到数据量达到最大时才通过网络一次发送全部数据,
 * 这种数据传输方式有益于大量数据的通信性能，典型的应用就是文件服务器.
 * 应用 Nagle算法在这种情况下就会产生问题,但是，如果你正在发送大量数据,
 * 你可以设置TCP_CORK选项禁用Nagle化, 其方式正好同 TCP_NODELAY相反
 * (TCP_CORK和 TCP_NODELAY是互相排斥的)
 *
 * 打开sendfile选项时,确定开启FreeBSD系统上的TCP_NOPUSH或Linux系统上的TCP_CORK功能.
 * 打开tcp_nopush后, 将会在发送响应时把整个响应包头放到一个TCP包中发送.
 */

/*
 * sendfile need it to be cork 1
 * Example from:
 * http://www.baus.net/on-tcp_cork
 */
bool SetTcpNoPush(const int fd, bool on) {
  MSF_INFO << "Socket, set Cork Flag FD " << fd << "to" << (on ? "ON" : "OFF");

  int cork = on;
#if defined(TCP_CORK)
  if (setsockopt(fd, SOL_TCP, TCP_CORK, &cork, sizeof(cork) < 0)) {
    return false;
  }
#elif defined(TCP_NOPUSH)
  if (setsockopt(fd, SOL_SOCKET, TCP_NOPUSH, &cork, sizeof(cork) < 0)) {
    return false;
  }
#endif
  return true;
}

/*
 * Enable the TCP_FASTOPEN feature for server side implemented in
 * Linux Kernel >= 3.7, for more details read here:
 * TCP Fast Open: expediting web services: http://lwn.net/Articles/508865/
 *
 * https://blog.csdn.net/u011130578/article/details/44515165
 * https://blog.csdn.net/qq_15902713/article/details/78565541
 */
bool SetTcpFastOpen(const int fd) {
#if defined(__linux__)
  int qlen = 5;
  if (setsockopt(fd, SOL_TCP, TCP_FASTOPEN, &qlen, sizeof(qlen)) < 0) {
    MSF_ERROR << "Could not set TCP_FASTOPEN.";
    return false;
  }

  // Call getsockopt to check if TFO was used.
  struct tcp_info info;
  socklen_t info_len = sizeof(info);
  errno = 0;
  if (getsockopt(fd, IPPROTO_TCP, TCP_INFO, &info, &info_len) != 0) {
    // errno is set from getsockopt
    return false;
  }
  return info.tcpi_options & TCPI_OPT_SYN_DATA;
#endif
  return true;
}

/*
TCP_DEFER_ACCEPT 优化 使用TCP_DEFER_ACCEPT可以减少用户程序hold的连接数,
也可以减少用户调用epoll_ctl和epoll_wait的次数，从而提高了程序的性能。
设置listen套接字的TCP_DEFER_ACCEPT选项后， 只当一个链接有数据时是才会
从accpet中返回（而不是三次握手完成)。
所以节省了一次读第一个http请求包的过程,减少了系统调用
查询资料，TCP_DEFER_ACCEPT是一个很有趣的选项，
Linux 提供的一个特殊 setsockopt,　在 accept 的 socket 上面,
只有当实际收到了数据，才唤醒正在 accept
的进程，可以减少一些无聊的上下文切换。代码如下。 val = 5;
setsockopt(srv_socket->fd, SOL_TCP, TCP_DEFER_ACCEPT, &val, sizeof(val));
里面 val 的单位是秒，注意如果打开这个功能，kernel 在 val 秒之内还没有
收到数据，不会继续唤醒进程，而是直接丢弃连接。
经过测试发现，设置TCP_DEFER_ACCEPT选项后，服务器受到一个CONNECT请求后，
操作系统不会Accept，也不会创建IO句柄。操作系统应该在若干秒，
(但肯定远远大于上面设置的1s) 后，会释放相关的链接。
但没有同时关闭相应的端口，所以客户端会一直以为处于链接状态。
如果Connect后面马上有后续的发送数据，那么服务器会调用Accept接收这个链接端口。
感觉了一下，这个端口设置对于CONNECT链接上来而又什么都不干的攻击方式处理很有效。
我们原来的代码都是先允许链接，然后再进行超时处理，比他这个有点Out了。
不过这个选项可能会导致定位某些问题麻烦。
timeout = 0表示取消 TCP_DEFER_ACCEPT选项
性能四杀手：内存拷贝，内存分配，进程切换，系统调用。
TCP_DEFER_ACCEPT 对性能的贡献，就在于减少系统调用了。
*/
bool SetTcpDeferAccept(const int fd) {
#if defined(__linux__)
  /* TCP_DEFER_ACCEPT tells the kernel to call defer accept() only after data
   * has arrived and ready to read */
  /* Defer Linux accept() up to for 1 second. */
  int timeout = 0;
  if (setsockopt(fd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &timeout, sizeof(int)) <
      0) {
    return false;
  }
#else
  /* Deferred accept() is not supported on AF_UNIX sockets. */
  (void)fd;
#endif
  return true;
}

/* https://www.cnblogs.com/cobbliu/p/4655542.html
 * Set TCP keep alive option to detect dead peers. The interval option
 * is only used for Linux as we are using Linux-specific APIs to set
 * the probe send time, interval, and count. */
bool SetKeepAlive(const int fd, bool on) {
  int flags = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags)) <
      0) {
    MSF_ERROR << "setsockopt(SO_KEEPALIVE).";
    return false;
  }

  int interval = 1;
  int val = 1;

  val = interval;

#ifdef __linux__
  /* Default settings are more or less garbage, with the keepalive time
   * set to 7200 by default on Linux. Modify settings to make the feature
   * actually useful. */

  /* Send first probe after interval. */
  val = interval;
  if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof(val)) < 0) {
    return false;
  }

  /* Send next probes after the specified interval. Note that we set the
   * delay as interval / 3, as we send three probes before detecting
   * an error (see the next setsockopt call). */
  val = interval / 3;
  if (val == 0) val = 1;
  if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, sizeof(val)) < 0) {
    return false;
  }

  /* Consider the socket in error state after three we send three ACK
   * probes without getting a reply. */
  val = 3;
  if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &val, sizeof(val)) < 0) {
    return false;
  }
#endif

  return true;
}

/**
 * https://blog.csdn.net/iteye_15898/article/details/81755762
 * */
bool SetLinger(const int fd, bool on) {
  struct linger l;
  l.l_onoff = 1;
  l.l_linger = 0;
  if (setsockopt(fd, SOL_SOCKET, SO_LINGER, (void *)&l, sizeof(l)) < 0) {
    MSF_ERROR << "Socket opt linger errno: " << strerror(errno);
    return false;
  }
  return true;
}

int BindAddress(const int fd, const struct sockaddr *addr,
                const socklen_t addrlen) {
  int ret = -1;
  ret = bind(fd, addr, addrlen);
  if (unlikely(ret < 0)) {
    int e = errno;
    switch (e) {
      case 0:
        MSF_ERROR << "Could not bind socket.";
        break;
      case EADDRINUSE:
        MSF_ERROR << "IP addr is in use.";
        break;
      case EADDRNOTAVAIL:
        MSF_ERROR << "IP addr maybe has been changed.";
        break;
      default:
        MSF_ERROR << "Could not bind UDP receive port errno: " << e;
        break;
    }
    return ret;
  }
  return 0;
}

int CreateTcpServer(const std::string &host, const uint16_t port,
                    const uint32_t proto, const uint32_t backlog) {
#ifdef WIN32
  WSADATA wsa_data;
  WSAStartup(0x0201, &wsa_data);
#endif

  int fd = -1;
  int ret = -1;
  struct addrinfo *ai;
  struct addrinfo *next;
  struct addrinfo hints = {.ai_flags = AI_PASSIVE, .ai_family = AF_UNSPEC};
  hints.ai_socktype = proto;

  hints.ai_socktype = SOCK_STREAM;
  // hints.ai_protocol = IPPROTO_TCP;
  char portBuff[NI_MAXSERV] = {0};
  snprintf(portBuff, sizeof(portBuff) - 1, "%d", port);

  MSF_INFO << "host: " << host << " port: " << port;

  /* Try with IPv6 if no IPv4 address was found. We do it in this order since
   * in a Redis client you can't afford to test if you have IPv6 connectivity
   * as this would add latency to every connect. Otherwise a more sensible
   * route could be: Use IPv6 if both addresses are available and there is IPv6
   * connectivity. */
  ret = getaddrinfo(host.c_str(), portBuff, &hints, &ai);
  if (unlikely(ret != 0)) {
    if (ret != EAI_SYSTEM) {
      MSF_ERROR << "Connect to " << host << " port " << port
                << " failed getaddrinfo: " << errno << " " << gai_strerror(ret);
    } else {
      perror("getaddrinfo()");
    }
    return false;
  }

  for (next = ai; next; next = next->ai_next) {
    fd = CreateSocket(next->ai_family, next->ai_socktype, next->ai_protocol);
    if (fd < 0) {
      /* getaddrinfo can return "junk" addresses,
       * we make sure at least one works before erroring.
       */
      if (errno == EMFILE || errno == ENFILE) {
        /* ...unless we're out of fds */
        perror("server_socket");
        freeaddrinfo(ai);
        return -1;
      }
      continue;
    }

    if (next->ai_family == AF_INET6) {
      if (!SetIpv6Only(fd, true)) {
        close(fd);
        continue;
      }
    }

    SetNonBlock(fd, true);
    SetReuseAddr(fd, true);
    SetReusePort(fd, true);

    if (IS_UDP(proto)) {
      SetSendBufferSize(fd, 1024);
    } else {
      SetLinger(fd, true);
      SetKeepAlive(fd, true);
      SetTcpNoDelay(fd, true);
    }

    if (BindAddress(fd, next->ai_addr, next->ai_addrlen) < 0) {
      if (errno != EADDRINUSE) {
        perror("bind()");
        freeaddrinfo(ai);
        close(fd);
        return -1;
      }
      close(fd);
      continue;
    } else {
      if (!IS_UDP(proto)) {
        /*
         * Enable TCP_FASTOPEN by default: if for some reason this call fail,
         * it will not affect the behavior of the server, in order to succeed,
         * Monkey must be running in a Linux system with Kernel >= 3.7 and the
         * tcp_fastopen flag enabled here:
         *
         *   # cat /proc/sys/net/ipv4/tcp_fastopen
         *
         * To enable this feature just do:
         *
         *   # echo 1 > /proc/sys/net/ipv4/tcp_fastopen
         */
        SetTcpFastOpen(fd);
        ret = listen(fd, backlog);
        if (ret < 0) {
          perror("listen()");
          freeaddrinfo(ai);
          close(fd);
          return false;
        }
      }
      if (next->ai_addr->sa_family == AF_INET ||
          next->ai_addr->sa_family == AF_INET6) {
        std::string protostr = IS_UDP(proto) ? "UDP" : "TCP";
        MSF_INFO << "conn protocol: " << protostr;
      }
    }
    break;
  }
  freeaddrinfo(ai);
  if (unlikely(nullptr == ai)) {
    close(fd);
    return -1;
  }
  return fd;
}

/**
 * https://www.cnblogs.com/cz-blog/p/4530641.html
 * https://blog.csdn.net/ET_Endeavoring/article/details/93532974
 * https://www.cnblogs.com/life2refuel/p/7337070.html
 * https://blog.csdn.net/nphyez/article/details/10268723
 * */
bool Connect(const int fd, const struct sockaddr *srvAddr, socklen_t addrLen,
             const int timeOutSec) {
#ifdef WIN32
  /* TODO: I don't know how to do this on Win32. Please send a patch. */
  if (connect(fd, srvAddr, addrLen) < 0) {
    if (WSAGetLastError() == WSAENETUNREACH) {
      MSF_ERROR << "Network is unreachable";
    }
    return false;
  }
  return true;
#else /* LINUX or DJGPP */
  int err;

  /* Default timeout in kernel is 75s */
  if (timeOutSec <= 0) {
    return (connect(fd, srvAddr, addrLen) == 0);
  } else {
    /* make socket non-blocking */
    if (!SetNonBlock(fd, true)) {
      return false;
    }

    /* start connect */
    if (connect(fd, srvAddr, addrLen) < 0) {
      int saveErrno = errno;
      switch (saveErrno) {
        case EINPROGRESS:
          MSF_ERROR << "Connect in progress";
          break;
        case EISCONN:
          MSF_ERROR << "Connect established yet";
          return true;
        case ENETDOWN:
        case ENETUNREACH:
          MSF_ERROR << "Network is unreachable";
        case EAGAIN:
        case EADDRINUSE:
          MSF_ERROR << "Addres is inused";
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
          MSF_ERROR << "Connect is refused";
        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
          MSF_ERROR << "Bad file descriptor";
        case EFAULT:
        case ENOTSOCK:
          MSF_ERROR << "Connect cannot ignore errno:" << strerror(saveErrno);
          return false;
        default:
          return false;
      }

#ifdef MSF_USE_SELECT
      struct timeval tv;
      fd_set rset;
      fd_set wset;
      tv.tv_sec = timeOutSec;
      tv.tv_usec = 0;
      FD_ZERO(&rset);
      FD_ZERO(&wset);
      FD_SET(fd, &rset);
      FD_SET(fd, &wset);

      /* wait for connect() to finish */
      if ((err = select(fd + 1, &rset, &wset, nullptr, &tv)) <= 0) {
        /* errno is already set if err < 0 */
        if (err == 0) {
          errno = ETIMEDOUT;
        }
        return false;
      }

      if (FD_ISSET(fd, &wset)) {
        /* test for success, set errno */
        if (SocketError(fd)) {
          return false;
        }
      }
#else
      struct pollfd pfd;
      pfd.fd = fd;
      pfd.events = POLLOUT | POLLERR;
      // https://blog.csdn.net/skypeng57/article/details/82743681
      // https://blog.csdn.net/cmh20161027/article/details/76401884
      err = poll(&pfd, 1, timeOutSec * 1000);
      if (err == 1 && pfd.revents == POLLOUT) {
        MSF_INFO << "Nonblock connect success.";
      } else {
        MSF_INFO << "Nonblock connect errno: " << errno;
        return false;
      }
#endif
    }
    /* restore blocking mode */
    if (!SetNonBlock(fd, false)) {
      return false;
    }
    return true;
  }
#endif /* UNIX */
}

int ConnectTcpServer(const std::string &host, const uint16_t port,
                     const uint32_t proto) {
#ifdef WIN32
  WSADATA wsa_data;
  WSAStartup(0x0201, &wsa_data);
#endif

  int fd = -1;
  int ret = -1;
  struct addrinfo *ai;
  struct addrinfo *next;
  struct addrinfo hints = {.ai_flags = AI_PASSIVE, .ai_family = AF_UNSPEC};
  hints.ai_socktype = proto;

  hints.ai_socktype = SOCK_STREAM;
  // hints.ai_protocol = IPPROTO_TCP;
  char portBuff[NI_MAXSERV] = {0};
  snprintf(portBuff, sizeof(portBuff) - 1, "%d", port);

  MSF_INFO << "Connect to host: " << host << " port: " << port;

  /* Try with IPv6 if no IPv4 address was found. We do it in this order since
   * in a Redis client you can't afford to test if you have IPv6 connectivity
   * as this would add latency to every connect. Otherwise a more sensible
   * route could be: Use IPv6 if both addresses are available and there is IPv6
   * connectivity. */
  ret = getaddrinfo(host.c_str(), portBuff, &hints, &ai);
  if (unlikely(ret != 0)) {
    if (ret != EAI_SYSTEM) {
      MSF_ERROR << "Connect to " << host << " port " << port
                << " failed getaddrinfo: " << errno << " " << gai_strerror(ret);
    } else {
      perror("getaddrinfo()");
    }
    return false;
  }

  for (next = ai; next; next = next->ai_next) {
    fd = CreateSocket(next->ai_family, next->ai_socktype, next->ai_protocol);
    if (fd < 0) {
      /* getaddrinfo can return "junk" addresses,
       * we make sure at least one works before erroring.
       */
      if (errno == EMFILE || errno == ENFILE) {
        /* ...unless we're out of fds */
        perror("server_socket");
        freeaddrinfo(ai);
        return -1;
      }
      continue;
    }

    if (next->ai_family == AF_INET6) {
      if (!SetIpv6Only(fd, true)) {
        close(fd);
        fd = -1;
        continue;
      }
    }

    SetNonBlock(fd, true);
    SetReuseAddr(fd, true);
    SetReusePort(fd, true);

    if (IS_UDP(proto)) {
      SetSendBufferSize(fd, 1024);
    } else {
      SetLinger(fd, true);
      SetKeepAlive(fd, true);
      SetTcpNoDelay(fd, true);
    }

    if (!Connect(fd, next->ai_addr, next->ai_addrlen, 5)) {
      close(fd);
      fd = -1;
      continue;
    }
    break;
  }
  freeaddrinfo(ai);
  if (unlikely(nullptr == ai)) {
    close(fd);
    return -1;
  }
  return fd;
}

/**
 * 参考:
 * https://blog.csdn.net/u011130578/article/details/44515165
 * https://blog.csdn.net/for_tech/article/details/54237556 */
int tfoSendMsg(const int fd, struct iovec *iov, int cnt, int flag) {
  flag |= MSG_FASTOPEN;
  return SendMsg(fd, iov, cnt, flag);
}
/************************* TCP SOCKET FEATURES ******************************/

/************************* UDP SOCKET FEATURES ******************************/
int SendTo(const int fd, const char *buf, const int len) {
  struct sockaddr_in sa;
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = inet_addr("192.168.0.110");
  sa.sin_port = htons(9999);
  int ret = sendto(fd, buf, len, 0, (struct sockaddr *)&sa, sizeof(sa));

  return ret;
}

int RecvFrom(const int fd, char *buf, const int len) {
  struct sockaddr_in sa;
  uint32_t addelen = sizeof(sa);
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = inet_addr("192.168.0.110");
  sa.sin_port = htons(9999);
  return recvfrom(fd, buf, (size_t)len, 0, (struct sockaddr *)&sa, &addelen);
}
/************************* UDP SOCKET FEATURES ******************************/

/************************* MUTICAST SOCKET FEATURES
 * ******************************/
// https://blog.csdn.net/timsley/article/details/50959182
// https://docs.oracle.com/cd/E38902_01/html/E38880/sockets-137.html

#include <linux/in6.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>

#include <iostream>
bool setMuticalastTTL(const int fd, const int ttl) {
  if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
    MSF_ERROR << "Setsockopt Error:IP_MULTICAST_TTL, errno:" << errno;
    return false;
  }
  return true;
}

bool setMuticalastIF(const int fd, const std::string &iface) {
  struct ifreq ifreq4;

  memset(&ifreq4, 0, sizeof(ifreq4));
  strlcpy(ifreq4.ifr_name, iface.c_str(), sizeof(ifreq4.ifr_name));
  (void)ioctl(fd, SIOCGIFADDR, &ifreq4);
  if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF,
                 &((struct sockaddr_in *)&ifreq4.ifr_addr)->sin_addr,
                 sizeof(struct in_addr))) {
    return false;
  }
  return true;
}

bool setMuticalastLoop(const int fd, uint32_t af, const int loop) {
  if (AF_INET == af) {
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) <
        0) {
      MSF_ERROR << "Setsockopt Error:IP_MULTICAST_LOOP, errno:" << errno;
      return false;
    }
  } else {
    if (setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &loop, sizeof(loop)) <
        0) {
      MSF_ERROR << "Setsockopt Error:IPV6_MULTICAST_LOOP, errno:" << errno;
      return false;
    }
  }
  return true;
}

bool addMuticalastMemershipv4(const int fd, const std::string &ip) {
  struct ip_mreq mreq;
  mreq.imr_multiaddr.s_addr = inet_addr(ip.c_str());
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);
  if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
    MSF_ERROR << "Setsockopt Error:IP_ADD_MEMBERSHIP, errno:" << errno;
    return false;
  }
  return true;
}

bool addMuticalastMemershipv6(const int fd, const std::string &ip) {
  struct sockaddr_in6 ipv6;
  struct ipv6_mreq mreq6;
  memset(&mreq6, 0, sizeof(struct ipv6_mreq));
  memcpy(&mreq6.ipv6mr_multiaddr, &ipv6.sin6_addr, sizeof(struct in6_addr));
  // mreq6.ipv6mr_interface = if_nametoindex(mreq6.ifr_name);
  if (setsockopt(fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char *)&mreq6,
                 sizeof(mreq6) < 0)) {
    return false;
  }
  return true;
}

bool dropMuticalastMemershipv4(const int fd, const std::string &ip) {
  struct ip_mreq mreq;
  struct sockaddr_in ipv4 = {0};
  // strncpy(mreq.ifr_name, "eth0", IFNAMSIZ);
  (void)ioctl(fd, SIOCGIFADDR, &mreq);
  mreq.imr_multiaddr.s_addr = ipv4.sin_addr.s_addr;
  // mreq.imr_interface.s_addr = ((struct sockaddr_in
  // *)&mreq.ifr_addr)->sin_addr.s_addr;
  mreq.imr_multiaddr.s_addr = inet_addr(ip.c_str());
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);
  if (setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
    MSF_ERROR << "Setsockopt Error:IP_DROP_MEMBERSHIP, errno:" << errno;
    return false;
  }
  return true;
}

bool dropMuticalastMemershipv6(const int fd, const std::string &ip) {
  struct sockaddr_in6 ipv6;
  struct ipv6_mreq merq6;
  // merq6.ipv6mr_multiaddr.s_addr = inet_addr(ip.c_str());
  // merq6.ipv6mr_interface.s_addr = htonl(INADDR_ANY);
  if (setsockopt(fd, IPPROTO_IPV6, IP_DROP_MEMBERSHIP, &merq6, sizeof(merq6)) <
      0) {
    MSF_ERROR << "Setsockopt Error:IP_DROP_MEMBERSHIP, errno:" << errno;
    return false;
  }
  memcpy(&merq6.ipv6mr_multiaddr, &ipv6.sin6_addr, sizeof(struct in6_addr));
  // merq6.ipv6mr_interface = if_nametoindex(merq6.ifr_name);
  if (setsockopt(fd, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, &merq6,
                 sizeof(merq6)) < 0) {
    MSF_ERROR << "Setsockopt Error:IPV6_DROP_MEMBERSHIP, errno:" << errno;
    return false;
  }
  return true;
}

int readMuticastData(const int fd, struct sockaddr_in *addr) {
  socklen_t addrlen = sizeof(*addr);
  char buff[256];
  while (true) {
    memset(buff, 0, 256);
    if (recvfrom(fd, buff, 256, 0, (struct sockaddr *)&addr, &addrlen) < 0) {
    }
  }
}

int CreateMuticastFd(const std::string &ip, const uint16_t port) {
  int sfd = -1;
  struct sockaddr_in addr;

  sfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sfd < 0) {
    MSF_ERROR << "create multicast socket failed: " << errno;
    return -1;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(ip.c_str());
  addr.sin_port = htons(port);

  setMuticalastTTL(sfd, 10);
  setMuticalastLoop(sfd, AF_INET, true);
  addMuticalastMemershipv4(sfd, ip);

  readMuticastData(sfd, &addr);

  dropMuticalastMemershipv4(sfd, ip);
  close(sfd);
  return sfd;
}
/************************* MUTICAST SOCKET FEATURES
 * ******************************/

/************************* UNIX SOCKET FEATURES ******************************/
int SetupAbsAddr(struct sockaddr_un &addr, const std::string &cliPath) {
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  memcpy(addr.sun_path, cliPath.c_str(), sizeof(addr.sun_path) - 1);

  unlink(addr.sun_path); /* in case it already exists */

  return offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path + 1) + 1;
}

/* create a UNIX domain stream socket */
int CreateUnixServer(const std::string &srvPath, const uint32_t backlog,
                     const uint32_t access_mask) {
  int fd;
  struct stat unix_stat;
  mode_t mode;
  int old_umask;
  struct sockaddr_un s_un;

  memset(&s_un, 0, sizeof(struct sockaddr_un));
  s_un.sun_family = AF_UNIX;

  fd = CreateUnixSocket();
  if (fd < 0) {
    return -1;
  }

  SetReuseAddr(fd, true);

  /* Clean up a previous socket file if we left it around
   */
  if (lstat(srvPath.c_str(), &unix_stat) == 0) {
    if (S_ISSOCK(unix_stat.st_mode))
      unlink(srvPath.c_str()); /* in case it already exists */
  }

  mode = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
  if (chmod((char *)srvPath.c_str(), mode) == -1) {
    MSF_WARN << "chmod access failed for " << srvPath;
  }

  memcpy(s_un.sun_path, srvPath.c_str(),
         std::min(sizeof(s_un.sun_path) - 1, srvPath.size()));
  // int len = offsetof(struct sockaddr_un, sun_path) + strlen(s_un.sun_path);

  old_umask = umask(~(access_mask & 0777));

  /* bind the name to the descriptor */
  if (bind(fd, (struct sockaddr *)&s_un, sizeof(s_un)) < 0) {
    umask(old_umask);
    MSF_ERROR << "Bind failed  for " << s_un.sun_path
              << "errno: " << strerror(errno);
    close(fd);
    return -1;
  }
  umask(old_umask);

  if (listen(fd, backlog) < 0) {
    /* tell kernel we're a server */
    MSF_ERROR << "listen call failed errno :" << strerror(errno);
    close(fd);
    return -1;
  }
  return fd;
}

int ConnectUnixServer(const std::string &srvPath, const std::string &cliPath) {
  int fd;
  fd = CreateUnixSocket();
  if (fd < 0) {
    return -1;
  }
  SetNonBlock(fd, true);

  struct sockaddr_un addr;
  SetupAbsAddr(addr, cliPath);

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    MSF_ERROR << "Bind failed for fd: " << fd;
    close(fd);
    return -1;
  }

  /* 0777 CLI_PERM */
  if (chmod(addr.sun_path, 0777) < 0) {
    MSF_ERROR << "Chmod failed for unix path:" << addr.sun_path;
    close(fd);
    return -1;
  }

  memset(addr.sun_path, 0, sizeof(addr.sun_path));
  snprintf(addr.sun_path, sizeof(addr.sun_path) - 1, "%s", srvPath.c_str());

  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    if (errno == EINPROGRESS) {
      return true;
    } else {
      MSF_ERROR << "Failed to connect : " << addr.sun_path << ":"
                << strerror(errno),
          close(fd);
      return -1;
    }
  }
  return fd;
}
/************************* UNIX SOCKET FEATURES ******************************/

/************************* EVENT SOCKET FEATURES ******************************/
/*
 * Glibc eventfd() wrapper always has the flags argument.	Glibc 2.7
 * and 2.8 eventfd() wrappers call the original eventfd() syscall
 * without the flags argument.  Glibc 2.9+ eventfd() wrapper at first
 * tries to call eventfd2() syscall and if it fails then calls the
 * original eventfd() syscall.  For this reason the non-blocking mode
 * is set separately.
 * Wrapper around eventfd on systems that provide it.	Unlike the system
 * eventfd, it always supports EVUTIL_EFD_CLOEXEC and EVUTIL_EFD_NONBLOCK as
 * flags.  Returns -1 on error or if eventfd is not supported.
 * flags:  EFD_NONBLOCK EFD_CLOEXEC EFD_SEMAPHORE.
 */

/*
 * The maximum value after write() to a eventfd() descriptor will
 * block or return EAGAIN is 0xFFFFFFFFFFFFFFFE, so the descriptor
 * can be read once per many notifications, for example, once per
 * 2^32-2 noticifcations.	Since the eventfd() file descriptor is
 * always registered in EPOLLET mode, epoll returns event about
 * only the latest write() to the descriptor.
 */
int CreateEventFd(const int initval, const int flags) {
  int fd = -1;
#if defined(EFD_CLOEXEC) && defined(EFD_NONBLOCK)
  fd = eventfd(initval, flags);
  if (fd >= 0 || flags == 0) {
    return fd;
  }
#endif

  fd = eventfd(initval, 0);
  if (fd < 0) {
    MSF_ERROR << "Failed to create event fd";
    return -1;
  }

  if (flags & EFD_CLOEXEC) {
    if (!SetCloseOnExec(fd)) {
      MSF_ERROR << "Failed to set event fd cloexec";
      close(fd);
      return -1;
    }
  }

  if (flags & EFD_NONBLOCK) {
    if (!SetCloseOnExec(fd)) {
      MSF_ERROR << "Failed to set event fd nonblock";
      close(fd);
      return -1;
    }
  }
  return fd;
}

/*
 * Every time that a write(2) is performed on an eventfd, the
 * value of the __u64 being written is added to "count" and a
 * wakeup is performed on "wqh". A read(2) will return the "count"
 * value to userspace, and will reset "count" to zero. The kernel
 * side eventfd_signal() also, adds to the "count" counter and
 * issue a wakeup.
 */
bool NotifyEvent(const int fd) {
  uint64_t u = 1;
  // eventfd_t u = 1;
  // ssize_t s = eventfd_write(efd, &u);
  ssize_t s = write(fd, &u, sizeof(u));
  if (s != sizeof(uint64_t)) {
    MSF_ERROR << "writing uint64_t to notify error.";
    return false;
  }
  return true;
}
bool ClearEvent(const int fd) {
  uint64_t u;
  // eventfd_t u;
  // ssize_t s = eventfd_read(efd, &u);
  ssize_t s = read(fd, &u, sizeof(u));
  if (s != sizeof(uint64_t)) {
    MSF_ERROR << "read uint64_t to notify error.";
    ;
    return false;
  }
  return true;
}

/************************* EVENT SOCKET FEATURES ******************************/

/************************* TIMER SOCKET FEATURES ******************************/
int CreateTimerFd() {
  int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  if (fd < 0) {
    MSF_ERROR << "create timer fd failed";
    if (errno != EINVAL && errno != ENOSYS) {
      /* These errors probably mean that we were
       * compiled with timerfd/TFD_* support, but
       * we're running on a kernel that lacks those.
       */
    }
    return -1;
  }
  return fd;
}

bool UpdateTimerFd(const int fd) {
  struct itimerspec new_value;
  struct timespec start;
  int max_expires = 0;

  if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
    MSF_FATAL << "failed to get clock realtime";
    return false;
  }

  /* Create a CLOCK_REALTIME absolute timer with initial
   * expiration and interval as specified in command line
   * 定时器是每个进程自己的, 不是在fork时继承的, 不会被传递给子进程 */
  new_value.it_value.tv_sec = start.tv_sec + max_expires;
  new_value.it_value.tv_nsec = start.tv_nsec;
  new_value.it_interval.tv_sec = 1;
  new_value.it_interval.tv_nsec = 0;

  // timerfd_settime(tmfd, 0, &new_value, NULL)
  if (timerfd_settime(fd, TFD_TIMER_ABSTIME, &new_value, NULL) == -1) {
    MSF_FATAL << "failed to set clock realtime";
    return false;
  }
  return true;
}
/************************* TIMER SOCKET FEATURES ******************************/

/************************* SIGNAL SOCKET FEATURES
 * ******************************/
/*
 * Glibc signalfd() wrapper always has the flags argument.	Glibc 2.7
 * and 2.8 signalfd() wrappers call the original signalfd() syscall
 * without the flags argument.	Glibc 2.9+ signalfd() wrapper at first
 * tries to call signalfd4() syscall and if it fails then calls the
 * original signalfd() syscall.  For this reason the non-blocking mode
 * is set separately.
 */
int CreateSigalFd(void) {
  sigset_t sigmask;
  if (sigprocmask(SIG_BLOCK, &sigmask, NULL) != 0) {
    MSF_ERROR << "set signal block failed with errno: " << errno;
    return -1;
  }

  int fd = signalfd(-1, &sigmask, 0);
  if (fd < 0) {
    MSF_ERROR << "create signal fd failed with errno: " << errno;
  }
  return fd;
}
/************************* SIGNAL SOCKET FEATURES
 * ******************************/

/************************* NETLINK SOCKET FEATURES
 * ******************************/

#define NETLINK_TEST 30
#define MSG_LEN 125
#define MAX_PLOAD 125

typedef struct _user_msg_info {
  struct nlmsghdr hdr;
  char msg[MSG_LEN];
} user_msg_info;

char *data;
uint32_t len;
struct sockaddr_nl src_addr;
struct sockaddr_nl dest_addr;
/*
* Netlink套接字是用以实现用户进程与内核进程通信的一种特殊的进程间通信(IPC),
* 也是网络应用程序与内核通信的最常用的接口.
* 使用netlink 进行应用与内核通信的应用很多:
* 包括:
    1. NETLINK_ROUTE:用户空间路由damon,如BGP,OSPF,RIP和内核包转发模块的通信信道
    用户空间路由damon通过此种netlink协议类型更新内核路由表
    2. NETLINK_FIREWALL:接收IPv4防火墙代码发送的包
    3. NETLINK_NFLOG:用户空间iptable管理工具和内核空间Netfilter模块的通信信道
    4. NETLINK_ARPD:用户空间管理arp表
    5. NETLINK_USERSOCK:用户态socket协议
    6. NETLINK_NETFILTER:netfilter子系统
    7. NETLINK_KOBJECT_UEVENT:内核事件向用户态通知
    8. NETLINK_GENERIC:通用netlink
*
*  Netlink 是一种在内核与用户应用间进行双向数据传输的非常好的方式,
*  用户态应用使用标准的 socket API 就可以使用 netlink 提供的强大功能,
*  内核态需要使用专门的内核 API 来使用 netlink
*  1. netlink是一种异步通信机制, 在内核与用户态应用之间传递的消息保存在socket
*  缓存队列中，发送消息只是把消息保存在接收者的socket的接收队列,
*  而不需要等待接收者收到消息, 它提供了一个socket队列来平滑突发的信息
*  2. 使用 netlink 的内核部分可以采用模块的方式实现,使用 netlink
的应用部分和内核部分没有编译时依赖
*  3. netlink 支持多播, 内核模块或应用可以把消息多播给一个netlink组,
*   属于该neilink 组的任何内核模块或应用都能接收到该消息,
*   内核事件向用户态的通知机制就使用了这一特性
*  4. 内核可以使用 netlink 首先发起会话
 5. netlink采用自己独立的地址编码, struct sockaddr_nl；
 6.每个通过netlink发出的消息都必须附带一个netlink自己的消息头,struct nlmsghdr
*
*  7. 用户态应用使用标准的 socket API有sendto(), recvfrom(),sendmsg(), recvmsg()
 在基于netlink的通信中,有两种可能的情形会导致消息丢失:
 1、内存耗尽,没有足够多的内存分配给消息
 2、缓存复写,接收队列中没有空间存储消息,这在内核空间和用户空间之间通信
        时可能会发生缓存复写在以下情况很可能会发生:
 3、内核子系统以一个恒定的速度发送netlink消息,但是用户态监听者处理过慢
 4、用户存储消息的空间过小
 如果netlink传送消息失败,那么recvmsg()函数会返回No buffer
spaceavailable(ENOBUFS)错误
*/

int CreateNLSocket() {
  struct sockaddr_nl saddr;

  /* 第一个参数必须是PF_NETLINK或者AF_NETLINK,
  第二个参数用SOCK_DGRAM和SOCK_RAW都没问题,
  第三个参数就是netlink的协议号*/

  int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_TEST);
  if (fd < 0) return -1;

  memset(&saddr, 0, sizeof(saddr));
  saddr.nl_family = AF_NETLINK;
  saddr.nl_pid = getpid();
  saddr.nl_groups = 0;

  /*
  nl_pid就是一个约定的通信端口，用户态使用的时候需要用一个非0的数字，
  一般来 说可以直接采用上层应用的进程ID（不用进程ID号码也没事，
  只要系统中不冲突的一个数字即可使用）。
  对于内核的地址，该值必须用0，也就是说，
  如果上层 通过sendto向内核发送netlink消息，peer addr中nl_pid必须填写0�
  */

  if (bind(fd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
    perror("bind() error.");
    close(fd);
    return -1;
  }
  SetNonBlock(fd, true);
  return fd;
}

int NLWrite(const int fd) {
  struct iovec iov[2];
  struct msghdr msg;
  struct nlmsghdr nlh = {0};

  iov[0].iov_base = &nlh;
  iov[0].iov_len = sizeof(nlh);
  iov[1].iov_base = data;
  iov[1].iov_len = NLMSG_SPACE(len) - sizeof(nlh);

  nlh.nlmsg_len = NLMSG_SPACE(len);
  nlh.nlmsg_pid = getpid();
  nlh.nlmsg_flags = 0;
  nlh.nlmsg_type = 0;

  memset(&msg, 0, sizeof(msg));
  msg.msg_name = (char *)&dest_addr;
  msg.msg_namelen = sizeof(dest_addr);
  msg.msg_iov = iov;
  msg.msg_iovlen = 2;

  return sendmsg(fd, &msg, 0);
}
int NLRead(const int fd) {
  struct iovec iov[2];
  struct msghdr msg;
  struct nlmsghdr nlh;

  iov[0].iov_base = &nlh;
  iov[0].iov_len = sizeof(nlh);
  iov[1].iov_base = data;
  iov[1].iov_len = len;

  memset(&msg, 0, sizeof(msg));
  msg.msg_name = (void *)&src_addr;
  msg.msg_namelen = sizeof(src_addr);
  msg.msg_iov = iov;
  msg.msg_iovlen = 2;

  return recvmsg(fd, &msg, MSG_DONTWAIT);
}
int NLSendto(const int fd) {
  struct sockaddr_nl daddr;
  memset(&daddr, 0, sizeof(daddr));
  daddr.nl_family = AF_NETLINK;
  daddr.nl_pad = 0;    /*always set to zero*/
  daddr.nl_pid = 0;    /*kernel's pid is zero*/
  daddr.nl_groups = 0; /*multicast groups mask, if unicast set to zero*/

  struct nlmsghdr *nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PLOAD));
  memset(nlh, 0, sizeof(struct nlmsghdr));
  nlh->nlmsg_len = NLMSG_SPACE(MAX_PLOAD);  // NLMSG_LENGTH(0);
  nlh->nlmsg_flags = 0;
  nlh->nlmsg_type = 0;
  nlh->nlmsg_seq = 0;
  nlh->nlmsg_pid = getpid();

  memcpy(NLMSG_DATA(nlh), data, len);
  int rc = sendto(fd, nlh, nlh->nlmsg_len, 0, (struct sockaddr *)&daddr,
                  sizeof(struct sockaddr_nl));
  if (!rc) {
    perror("sendto error.");
    close(fd);
    exit(-1);
  }
  return rc;
}
int NLRecvfrom(const int fd) {
  user_msg_info u_info;
  memset(&u_info, 0, sizeof(u_info));

  struct sockaddr_nl daddr;

  memset(&daddr, 0, sizeof(daddr));
  daddr.nl_family = AF_NETLINK;
  daddr.nl_pid = 0;  // to kernel
  daddr.nl_groups = 0;

  socklen_t sock_len = sizeof(struct sockaddr_nl);
  int ret = recvfrom(fd, &u_info, sizeof(user_msg_info), 0,
                     (struct sockaddr *)&daddr, &sock_len);
  if (!ret) {
    perror("recv form kernel error.");
    close(fd);
    return -1;
  }
  return ret;
}

/************************* NETLINK SOCKET FEATURES
 * ******************************/
}  // namespace SOCK
}  // namespace MSF