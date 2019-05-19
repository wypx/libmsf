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

#include <net/route.h>
#include <net/ethernet.h>
#include <linux/sockios.h> 
#include <resolv.h>

#define __KERNEL__
#include <linux/ethtool.h>
#undef __KERNEL__

/*
 * Set Net Config
 * 1.Use system() or exec*() to call ifconfig and route.
 *   Advantage:	Simple
 *   Disadvantage: ineffective, but depend on ifconfig or route command.
 * 2.Create Socket,use iocrl().
 *   Advantage:	effective
 *   Disadvantage: diffcult.
 *
 *  Get Net Config
 *  1.Use popen(), other side execute ifconfig and route,
 *    this side recv and parse.
 *  2.Use fopen() to open /proc/net/route, get gateway.
 *    effective than ifconfig and route command.
 *  3. Create Socket,use iocrl().
*/

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
    int shutdown(int sockfd,int howto);  //返回成功为0，出错为-1.    该函数的行为依赖于howto的值
    1.SHUT_RD：值为0，关闭连接的读这一半。
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

/* Server features: depends on server setup and Linux Kernel version */
#define KERNEL_TCP_FASTOPEN      1
#define KERNEL_SO_REUSEPORT      2
#define KERNEL_TCP_AUTOCORKING   4

#define MSF_MOD_NETWORK "NETWORK"
#define MSF_NETWORK_LOG(level, ...) \
    msf_log_write(level, MSF_MOD_NETWORK, MSF_FUNC_FILE_LINE, __VA_ARGS__)

static s32 kernel_features = 1;
static s32 portreuse = 1;

const struct in6_addr g_any6addr = IN6ADDR_ANY_INIT; 
static u32 old_gateway = 0;

/*
* Checks validity of an IP address string based on the version
* AF_INET6 AF_INET
*/
s32 msf_isipaddr(const s8 *ip, u32 af_type) {
    s8  addr[sizeof(struct in6_addr)];
    s32 len = sizeof(addr);

#ifdef WIN32
    if (WSAStringToAddress(ip, af_type, NULL, PADDR(addr), &len) == 0)
        return 1;
#else /*~WIN32*/
    if (inet_pton(af_type, ip, addr) == 1)
        return 1;
#endif /*WIN32*/

    return 0;
}

s32 msf_sockaddr_cmp(const struct sockaddr *sa1, const struct sockaddr *sa2) {

    if (sa1->sa_family != sa2->sa_family)
        return -1;

    if (sa1->sa_family == AF_INET) {
        const struct sockaddr_in *sin1, *sin2;
        sin1 = (const struct sockaddr_in *)sa1;
        sin2 = (const struct sockaddr_in *)sa2;

        if ((sin1->sin_addr.s_addr == sin2->sin_addr.s_addr) &&
            ((s32)sin1->sin_port == (s32)sin2->sin_port)) {
            return 0;
        }
        return -1;
    }
#ifdef AF_INET6
    else if (sa1->sa_family == AF_INET6) {
        const struct sockaddr_in6 *sin1, *sin2;
        sin1 = (const struct sockaddr_in6 *)sa1;
        sin2 = (const struct sockaddr_in6 *)sa2;
        if ((0 == memcmp(sin1->sin6_addr.s6_addr, sin2->sin6_addr.s6_addr, 16)) &&
                ((s32)sin1->sin6_port == (s32)sin2->sin6_port))
            return 0;
        else
            return 0;
    }
#endif
    return 1;
}

u16 msf_sockaddr_port(struct sockaddr_storage *addr)
{
    u16 port;

    if (addr->ss_family == AF_INET6) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *) addr;
        port = addr6->sin6_port;
    } else {
        /* Note: this might be AF_UNSPEC if it is the sequence number of
        * a virtual server in a virtual server group */
        struct sockaddr_in *addr4 = (struct sockaddr_in *) addr;
        port = addr4->sin_port;
    }

    return port;
}

/**
* linux率莟cp_cork,上面的意思就是说,当使用sendfile函数时,
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
* Example from:
* http://www.baus.net/on-tcp_cork
*/

s32 msf_socket_cork_flag(s32 fd, u32 state) {
    MSF_NETWORK_LOG(DBG_ERROR, "Socket, set Cork Flag FD %i to %s", fd, (state ? "ON" : "OFF"));

#if defined (TCP_CORK)
    return setsockopt(fd, SOL_TCP, TCP_CORK, &state, sizeof(state));
#elif defined (TCP_NOPUSH)
    return setsockopt(fd, SOL_SOCKET, TCP_NOPUSH, &state, sizeof(state));
#endif
}

s32 msf_socket_blocking(s32 fd) {
#ifdef WIN32
    unsigned long nonblocking = 0;
    return ioctlsocket(fd, FIONBIO, &val);
#else
    s32 val = fcntl(fd, F_GETFL, 0);
    if (fcntl(fd, F_SETFL, val & ~O_NONBLOCK) == -1) {
        return -1;
    }
#endif
    return 0;
}

/*
* ioctl(FIONBIO) sets a non-blocking mode with the single syscall
* while fcntl(F_SETFL, O_NONBLOCK) needs to learn the current state
* using fcntl(F_GETFL).
*
* ioctl() and fcntl() are syscalls at least in FreeBSD 2.x, Linux 2.2
* and Solaris 7.
*
* ioctl() in Linux 2.4 and 2.6 uses BKL, however, fcntl(F_SETFL) uses it too.
*/
s32 msf_socket_nonblocking(s32 fd) {
#ifdef _WIN32
    {
        s32 nonblocking = 1;
        if (ioctlsocket(fd, FIONBIO, &nonblocking) == SOCKET_ERROR) {
            MSF_NETWORK_LOG(DBG_ERROR, fd, "fcntl(%d, F_GETFL)", (int)fd);
            return -1;
        }
    }
#else
    {
    s32 flags;
    if ((flags = fcntl(fd, F_GETFL, NULL)) < 0) {
        MSF_NETWORK_LOG(DBG_ERROR, "fcntl(%d, F_GETFL)", fd);
        return -1;
    }
    if (!(flags & O_NONBLOCK)) {
        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
                MSF_NETWORK_LOG(DBG_ERROR, "fcntl(%d, F_SETFL)", fd);
                return -1;
            }
        }

        fcntl(fd, F_SETFD, FD_CLOEXEC);
    }
#endif
    return 0;
}


//http://blog.csdn.net/yangzhao0001/article/details/48003337

/* Set the socket send timeout (SO_SNDTIMEO socket option) to the specified
* number of milliseconds, or disable it if the 'ms' argument is zero. */
s32 msf_socket_timeout(s32 fd, u32 ms) {

    struct timeval tv;

    tv.tv_sec = ms/1000;
    tv.tv_usec = (ms%1000)*1000;

    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv) < 0 )) {
        MSF_NETWORK_LOG(DBG_ERROR, "setsocketopt SO_SNDTIMEO errno %d.", errno);
        //return -1;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv) < 0 )) {
        MSF_NETWORK_LOG(DBG_ERROR, "setsocketopt failed errno %d.", errno);
    }

    return 0;
}

s32 msf_socket_reuseaddr(s32 fd) {

#if defined(SO_REUSEADDR) && !defined(_WIN32)
    s32 one = 1;
    /* REUSEADDR on Unix means, "don't hang on to this address after the
    * listener is closed."  On Windows, though, it means "don't keep other
    * processes from binding to this address while we're using it. */
    return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void*) &one, (s32)sizeof(one));
#else
    return 0;
#endif
}

s32 msf_socket_reuseport(s32 fd) {
#if defined __linux__ && defined(SO_REUSEPORT)
    s32 on = 1;
    /* REUSEPORT on Linux 3.9+ means, "Multiple servers (processes or
    * threads) can bind to the same port if they each set the option. */
    return setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &on, (s32)sizeof(on));
#endif
}

s32 msf_socket_linger(s32 fd) {
    struct linger l;
    l.l_onoff = 1;
    l.l_linger = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_LINGER, 
        (void*)&l, sizeof(l)) < 0) {
        MSF_NETWORK_LOG(DBG_ERROR, "Socket opt linger errno: %s.", strerror(errno));
        return -1;
    }
    return 0;
}

/* https://www.cnblogs.com/cobbliu/p/4655542.html
* Set TCP keep alive option to detect dead peers. The interval option
* is only used for Linux as we are using Linux-specific APIs to set
* the probe send time, interval, and count. */
s32 msf_socket_alive(s32 fd) {
    s32 flags = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void*)&flags, sizeof(flags)) < 0) {
        MSF_NETWORK_LOG(DBG_ERROR, "setsockopt(SO_KEEPALIVE).");
        return -1;
    }

    s32 interval = 1;
    s32 val = 1;

    val = interval;

#ifdef __linux__
    /* Default settings are more or less garbage, with the keepalive time
    * set to 7200 by default on Linux. Modify settings to make the feature
    * actually useful. */

    /* Send first probe after interval. */
    val = interval;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof(val)) < 0) {
        return -1;
    }

    /* Send next probes after the specified interval. Note that we set the
    * delay as interval / 3, as we send three probes before detecting
    * an error (see the next setsockopt call). */
    val = interval/3;
    if (val == 0) val = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, sizeof(val)) < 0) {
        return -1;
    }

    /* Consider the socket in error state after three we send three ACK
    * probes without getting a reply. */
    val = 3;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &val, sizeof(val)) < 0) {
        return -1;
    }
#endif

    return 0;
}

s32 msf_socket_closeonexec(s32 fd)
{
#if !defined(_WIN32) 
    s32 flags;
    if ((flags = fcntl(fd, F_GETFD, NULL)) < 0) {
        return -1;
    }
    if (!(flags & FD_CLOEXEC)) {
        if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1) {
            return -1;
        }
    }
#endif
    return 0;
}

void network_accept_tcp_nagle_disable (const int fd)
{
    static int noinherit_tcpnodelay = -1;
    int opt;

    if (!noinherit_tcpnodelay) /* TCP_NODELAY inherited from listen socket */
        return;

    if (noinherit_tcpnodelay < 0) {
    socklen_t optlen = sizeof(opt);
    if (0 == getsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, &optlen)) {
    noinherit_tcpnodelay = !opt;
        if (opt)			/* TCP_NODELAY inherited from listen socket */
            return;
        }
    }

    opt = 1;
    (void)setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
}

/*
Nagle算法的基本定义是任意时刻，最多只能有一个未被确认的小段。 所谓“小段”，指的是小于MSS尺寸的数据块，所谓“未被确认”，是指一
个数据块发送出去后，没有收到对方发送的ACK确认该数据已收到。
Nagle算法的规则（可参考tcp_output.c文件里tcp_nagle_check函数注释）：
（1）如果包长度达到MSS，则允许发送；
（2）如果该包含有FIN，则允许发送；
（3）设置了TCP_NODELAY选项，则允许发送；
（4）未设置TCP_CORK选项时，若所有发出去的小数据包（包长度小于MSS）均被确认，则允许发送； 
（5）上述条件都未满足，但发生了超时（一般为200ms），则立即发送。
Nagle算法只允许一个未被ACK的包存在于网络，它并不管包的大小，因此它事实上就是一个扩展的停-等协议，只不过它是基于包停-等的，而不
是基于字节停-等的。Nagle算法完全由TCP协议的ACK机制决定，这会带来一些问题，比如如果对端ACK回复很快的话，Nagle事实上不会拼接太多
的数据包，虽然避免了网络拥塞，网络总体的利用率依然很低。
TCP链接的过程中，默认开启Nagle算法，进行小包发送的优化。
2. TCP_NODELAY 选项
默认情况下，发送数据采用Negale 算法。这样虽然提高了网络吞吐量，但是实时性却降低了，在一些交互性很强的应用程序来说是不
允许的，使用TCP_NODELAY选项可以禁止Negale 算法。
此时，应用程序向内核递交的每个数据包都会立即发送出去。需要注意的是，虽然禁止了Negale 算法，但网络的传输仍然受到TCP确认延迟机制的影响。
3. TCP_CORK 选项 (tcp_nopush = on 会设置调用tcp_cork方法，配合sendfile 选项仅在使用sendfile的时候才开启)
所谓的CORK就是塞子的意思，形象地理解就是用CORK将连接塞住，使得数据先不发出去，等到拔去塞子后再发出去。设置该选项后，内核
会尽力把小数据包拼接成一个大的数据包（一个MTU）再发送出去，当然若一定时间后（一般为200ms，该值尚待确认），内核仍然没有组
合成一个MTU时也必须发送现有的数据（不可能让数据一直等待吧）。
然而，TCP_CORK的实现可能并不像你想象的那么完美，CORK并不会将连接完全塞住。内核其实并不知道应用层到底什么时候会发送第二批
数据用于和第一批数据拼接以达到MTU的大小，因此内核会给出一个时间限制，在该时间内没有拼接成一个大包（努力接近MTU）的话，内
核就会无条件发送。也就是说若应用层程序发送小包数据的间隔不够短时，TCP_CORK就没有一点作用，反而失去了数据的实时性（每个小
包数据都会延时一定时间再发送）。
4. Nagle算法与CORK算法区别
Nagle算法和CORK算法非常类似，但是它们的着眼点不一样，Nagle算法主要避免网络因为太多的小包（协议头的比例非常之大）而拥塞，而CORK
算法则是为了提高网络的利用率，使得总体上协议头占用的比例尽可能的小。如此看来这二者在避免发送小包上是一致的，在用户控制的层面上，
Nagle算法完全不受用户socket的控制，你只能简单的设置TCP_NODELAY而禁用它，CORK算法同样也是通过设置或者清除TCP_CORK使能或者禁用之，
然而Nagle算法关心的是网络拥塞问题，只要所有的ACK回来则发包，而CORK算法却可以关心内容，在前后数据包发送间隔很短的前提下（很重要，
否则内核会帮你将分散的包发出），即使你是分散发送多个小数据包，你也可以通过使能CORK算法将这些内容拼接在一个包内，如果此时用Nagle
算法的话，则可能做不到这一点。
naggle(tcp_nodelay设置)算法，只要发送出去一个包，并且受到协议栈ACK应答，内核就会继续把缓冲区的数据发送出去。
core(tcp_core设置)算法，受到对方应答后，内核首先检查当前缓冲区中的包是否有1500，如果有则直接发送，如果受到应答的时候还没有1500，则
等待200ms，如果200ms内还没有1500字节，则发送
*/ //参考http://m.blog.csdn.net/blog/c_cyoxi/8673645

s32 msf_socket_tcp_nodelay(const s32 fd) {

    s32 on = 1;

    if (setsockopt(fd, SOL_TCP, TCP_NODELAY, &on, sizeof(on)) < 0) {
        MSF_NETWORK_LOG(DBG_ERROR, "setsockopt TCP_NODELAY: %s", strerror(errno));
        return -1;
    };
    return 0;
}

int ngx_tcp_nopush(s32 s)
{
    int  cork;

    cork = 1;

    return setsockopt(s, IPPROTO_TCP, TCP_CORK,
       (const void *) &cork, sizeof(int)); //ѡ�����ʹ��sendfile��ʱ��ſ���
}

int
ngx_tcp_push(s32 s)//ѡ�����ʹ��sendfile��ʱ��ſ���
{
    int  cork;

    cork = 0;

    return setsockopt(s, IPPROTO_TCP, TCP_CORK,
        (const void *) &cork, sizeof(int));
}


/*
* Enable the TCP_FASTOPEN feature for server side implemented in
* Linux Kernel >= 3.7, for more details read here:
*
*  TCP Fast Open: expediting web services: http://lwn.net/Articles/508865/
*/
s32 msf_socket_tcp_fastopen(s32 fd) {
#if defined (__linux__)
    s32 qlen = 5;
    return setsockopt(fd, SOL_TCP, TCP_FASTOPEN, &qlen, sizeof(qlen));
#endif

    (void) fd;
    return -1;
}

/* 
TCP_DEFER_ACCEPT 优化 使用TCP_DEFER_ACCEPT可以减少用户程序hold的连接数,
也可以减少用户调用epoll_ctl和epoll_wait的次数，从而提高了程序的性能。
设置listen套接字的TCP_DEFER_ACCEPT选项后， 只当一个链接有数据时是才会
从accpet中返回（而不是三次握手完成)。
所以节省了一次读第一个http请求包的过程,减少了系统调用
查询资料，TCP_DEFER_ACCEPT是一个很有趣的选项，
Linux 提供的一个特殊 setsockopt,　在 accept 的 socket 上面,
只有当实际收到了数据，才唤醒正在 accept 的进程，可以减少一些无聊的上下文切换。代码如下。
val = 5;
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

s32 msf_socket_tcp_defer_accept(s32 fd) {
#if defined (__linux__)
    /* TCP_DEFER_ACCEPT tells the kernel to call defer accept() only after data
    * has arrived and ready to read */
    /* Defer Linux accept() up to for 1 second. */
    s32 timeout = 0;
    return setsockopt(fd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &timeout, sizeof(s32));
#else
    /* Deferred accept() is not supported on AF_UNIX sockets. */
    (void) fd;
    return -1;
#endif
}

static s32 parse_numeric_servname(const s8 *servname) {
    s32 n;
    s8 *endptr=NULL;
    n = (int) strtol(servname, &endptr, 10);
    if (n>=0 && n <= 65535 && servname[0] && endptr && !endptr[0])
        return n;
    else
        return -1;
}

/** Parse a service name in 'servname', which can be a decimal port.
* Return the port number, or -1 on error.
*/
static s32 msf_parse_servname(const s8 *servname, const s8 *protocol,
const struct addrinfo *hints)
{
    s32 n = parse_numeric_servname(servname);
    if (n>=0)
    return n;
#if defined(MSF_HAVE_GETSERVBYNAME) || defined(_WIN32)
    if (!(hints->ai_flags & AI_NUMERICSERV)) {
        struct servent *ent = getservbyname(servname, protocol);
        if (ent) {
            return ntohs(ent->s_port);
        }
    }
#endif
    return -1;
}

/* Return a string corresponding to a protocol number that we can pass to
* getservyname.  */
static const s8 *msf_unparse_protoname(s32 proto)
{
    switch (proto) {
    case 0:
        return NULL;
    case IPPROTO_TCP:
        return "tcp";
    case IPPROTO_UDP:
        return "udp";
#ifdef IPPROTO_SCTP
    case IPPROTO_SCTP:
        return "sctp";
#endif
    default:
#ifdef MSF_HAVE_GETPROTOBYNUMBER
    {
    struct protoent *ent = getprotobynumber(proto);
    if (ent)
        return ent->p_name;
    }
#endif
    return NULL;
    }
}

s32 msf_socketpair(s32 family, s32 type, s32 protocol, s32 fd[2]){
    return socketpair(family, type, protocol, fd);
}

/*
* Every time that a write(2) is performed on an eventfd, the
* value of the __u64 being written is added to "count" and a
* wakeup is performed on "wqh". A read(2) will return the "count"
* value to userspace, and will reset "count" to zero. The kernel
* side eventfd_signal() also, adds to the "count" counter and
* issue a wakeup.
*/
s32 msf_eventfd_notify(s32 efd) {

    u64 u = 1;
    //eventfd_t u = 1;
    //ssize_t s = eventfd_write(efd, &u);
    ssize_t s = write(efd, &u, sizeof(u));  
    if (s != sizeof(u64)) {
        MSF_NETWORK_LOG(DBG_ERROR, "Writing(%ld) ret(%ld) to notify error.", u, s);
        return -1;
    } else {
        MSF_NETWORK_LOG(DBG_INFO, "Writing(%ld) ret(%ld) to notify successful.", u, s);
    }
    return 0;
}

s32 msf_eventfd_clear(s32 efd) {

    u64 u;
    //eventfd_t u;
    //ssize_t s = eventfd_read(efd, &u); 
    ssize_t s = read(efd, &u, sizeof(u));  
    if (s != sizeof(u64)) {
        MSF_NETWORK_LOG(DBG_ERROR, "Read(%ld) cnt(%ld) to thread clear error.", u, s);
        return -1;
    } else {
        MSF_NETWORK_LOG(DBG_INFO, "Read(%ld) cnt(%ld) to thread clear successful.", u, s);
    }
    return 0;
}

/* Wrapper around eventfd on systems that provide it.	Unlike the system
* eventfd, it always supports EVUTIL_EFD_CLOEXEC and EVUTIL_EFD_NONBLOCK as
* flags.  Returns -1 on error or if eventfd is not supported.
* flags:  EFD_NONBLOCK EFD_CLOEXEC EFD_SEMAPHORE.
*/
s32 msf_eventfd(u32 initval, s32 flags) {

#if defined(MSF_HAVE_EVENTFD) && defined(MSF_HAVE_SYS_EVENTFD_H)
    s32 r;
#if defined(EFD_CLOEXEC) && defined(EFD_NONBLOCK)
    r = eventfd(initval, flags);
    if (r >= 0 || flags == 0) {
        return r;
    }
#endif
    r = eventfd(initval, 0);
    if (r < 0) {
        MSF_NETWORK_LOG(DBG_ERROR, "Failed to create event fd, errno(%d).", errno);
        return r;
    }

    if (flags & EFD_CLOEXEC) {
        if (msf_socket_closeonexec(r) < 0) {
            MSF_NETWORK_LOG(DBG_ERROR, "Failed to set event fd cloexec, errno(%d).", errno);
            sclose(r);
            return -1;
        }
    }

    if (flags & EFD_NONBLOCK) {
        if (msf_socket_nonblocking(r) < 0) {
            MSF_NETWORK_LOG(DBG_ERROR, "Failed to set event fd nonblock, errno(%d).", errno);
            sclose(r);
            return -1;
        }
    }
    return r;
#else
    return -1;
#endif
}

s32 msf_timerfd(u32 initval, s32 flags) {

    s32 timerfd = -1;

    timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK|TFD_CLOEXEC);
    if (timerfd < 0) {
        if (errno != EINVAL && errno != ENOSYS) {
            /* These errors probably mean that we were
             * compiled with timerfd/TFD_* support, but
             * we're running on a kernel that lacks those.
             */
        }
        timerfd = -1;
    }
    return timerfd;
}


s32 msf_socket_create(u32 domain, u32 type, u32 protocol) {
    s32 fd = -1;

#ifdef SOCK_CLOEXEC
    fd = socket(domain, type | SOCK_CLOEXEC, protocol);
#else
    fd = socket(domain, type, protocol);
    socket_closeonexec(fd);
#endif

    if (unlikely(fd == -1)) {
        perror("socket");
        return -1;
    }
    return fd;
}

s32 msf_socket_bind(s32 fd, const struct sockaddr *addr,
    socklen_t addrlen, u32 backlog)
{
    s32 ret = -1;

    ret = bind(fd, addr, addrlen);
    if (unlikely(ret < 0)) {
    s32 e = errno;
    switch (e) {
        case 0:
            MSF_NETWORK_LOG(DBG_ERROR, "Could not bind socket.");
            break;
        case EADDRINUSE:
            MSF_NETWORK_LOG(DBG_ERROR, "Port %d for receiving UDP is in use.", 8888);
            break;
        case EADDRNOTAVAIL:
            break;
        default:
            MSF_NETWORK_LOG(DBG_ERROR, "Could not bind UDP receive port errno:%d.", e);
            break;
        }
        return -1;
    }

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

    if (kernel_features & KERNEL_TCP_FASTOPEN) {
        ret = msf_socket_tcp_fastopen(fd);
        if (unlikely(ret == -1)) {
            MSF_NETWORK_LOG(DBG_ERROR, "Could not set TCP_FASTOPEN.");
        }
    }

    ret = listen(fd, backlog);
    if (unlikely(ret < 0)) {
        return -1;
    }

    return 0;
}

/* Network helpers */
void msf_socket_debug(s32 fd) {

    s32 rc;
    s8 ip[64];
    struct sockaddr_in *sin = NULL;
    struct sockaddr_in6 *sin6 = NULL;
    struct sockaddr_un* cun = NULL;
    struct sockaddr_storage cliaddr;
    socklen_t addrlen = sizeof(cliaddr);

    memset(ip, 0, sizeof(ip));
    memset(&cliaddr, 0, sizeof(cliaddr));

    rc = getsockname(fd, (struct sockaddr *)&cliaddr, &addrlen);
    if (rc < 0) {
        MSF_NETWORK_LOG(DBG_ERROR, "Conn getsockname failed, errno(%d).", errno);
        return;
    }

    if (cliaddr.ss_family == AF_UNIX) { 
        
        cun = SINU(&cliaddr);
        MSF_NETWORK_LOG(DBG_DEBUG, "Unix local(%s) clifd(%d).", cun->sun_path, fd);

    } else if (cliaddr.ss_family == AF_INET) {

        sin = SIN(&cliaddr);

        /* inet_ntoa(sin->sin_addr) */
        inet_ntop(cliaddr.ss_family, &sin->sin_addr, ip, sizeof(ip));
        MSF_NETWORK_LOG(DBG_DEBUG, "Network ipv4 local(%s) port(%d).", ip, ntohs(sin->sin_port));

    } else if (cliaddr.ss_family == AF_INET6) {

        sin6 = SIN6(&cliaddr);
        inet_ntop(cliaddr.ss_family, &sin6->sin6_addr, ip, sizeof(ip));
        MSF_NETWORK_LOG(DBG_DEBUG, "Network ipv6 local(%s) port(%d).", ip, ntohs(sin6->sin6_port));
    }

    memset(ip, 0, sizeof(ip));
    memset(&cliaddr, 0, sizeof(cliaddr));
    rc = getpeername(fd, (struct sockaddr *)&cliaddr, &addrlen);
    if (rc < 0) {
        MSF_NETWORK_LOG(DBG_ERROR, "Conn getpeername failed, errno(%d).", errno);
        return;
    }
    
    if (cliaddr.ss_family == AF_UNIX) { 

        cun = SINU(&cliaddr);
        MSF_NETWORK_LOG(DBG_DEBUG, "Unix peer[%s] clifd[%d].", cun->sun_path, fd);

    } else if (cliaddr.ss_family == AF_INET) {

        sin = SIN(&cliaddr);
        /* inet_ntoa(sin->sin_addr) */
        inet_ntop(cliaddr.ss_family, &sin->sin_addr, ip, sizeof(ip));
        MSF_NETWORK_LOG(DBG_DEBUG, "Network ipv4 peer[%s] port[%d].", ip, ntohs(sin->sin_port));

    } else if (cliaddr.ss_family == AF_INET6) {

        sin6 = SIN6(&cliaddr);
        inet_ntop(cliaddr.ss_family, &sin6->sin6_addr, ip, sizeof(ip));
        MSF_NETWORK_LOG(DBG_DEBUG, "Network ipv6 peer[%s] port[%d].",  ip, ntohs(sin6->sin6_port));
    }
}

s32 msf_connect_unix_socket (const s8 *cname, const s8 *sname) {
    s32 fd  = -1;
    s32 len =   0;
    struct sockaddr_un addr;

    if (unlikely(!cname || !sname)) {
        goto err;
    }

    fd = msf_socket_create(AF_UNIX, SOCK_STREAM, 0);
    if (unlikely(fd < 0)) {
        MSF_NETWORK_LOG(DBG_ERROR, "Failed to open socket: %s.", 
                    strerror (errno));
        goto err;
    }

    if (unlikely(msf_socket_nonblocking(fd) < 0 ||
                    msf_socket_linger(fd) < 0)) {
        MSF_NETWORK_LOG(DBG_ERROR,
            "Socket set noblock,linger,alive: %s.", strerror (errno));
        goto err;
    }

    msf_memzero(&addr, sizeof(addr));
    addr.sun_family = AF_UNIX;

    snprintf(addr.sun_path, sizeof(addr.sun_path) -1, "%s", cname);

    len = offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path);
    unlink(addr.sun_path);    /* in case it already exists */

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        MSF_NETWORK_LOG(DBG_ERROR, "Bind failed  errno %d fd %d.", errno, fd);
        goto err;
    }

    /* 0777 CLI_PERM */
    if (chmod(addr.sun_path, 0777) < 0) {
        MSF_NETWORK_LOG(DBG_ERROR, "Chmod sun_path %s failed.", addr.sun_path);
        goto err;
    }

    msf_memzero(addr.sun_path, sizeof(addr.sun_path));
    snprintf(addr.sun_path, sizeof(addr.sun_path) - 1, "%s", sname);

    if (connect (fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        if (errno == EINPROGRESS) 
            return fd;
        else {
            MSF_NETWORK_LOG(DBG_ERROR, "Failed to connect %s: %s.", 
                addr.sun_path, strerror(errno));
            goto err;
        }
    }

    return fd;
    err:
    sclose(fd);
    return -1;
}
 
s32 msf_connect_host(const s8 *host, const s8 *port)
{
    s32 flag = 1;
    s32 fd  = -1;
    s32 rc = -1;
    struct addrinfo hints = { 0, };
    struct addrinfo *res, *r;

    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    /* Try with IPv6 if no IPv4 address was found. We do it in this order since
    * in a Redis client you can't afford to test if you have IPv6 connectivity
    * as this would add latency to every connect. Otherwise a more sensible
    * route could be: Use IPv6 if both addresses are available and there is IPv6
    * connectivity. */
    rc = getaddrinfo (host, port, &hints, &res);
    if (unlikely(rc < 0)) {
        printf ("Connect to %s port %s failed (getaddrinfo: %s).",
            host, port, strerror(errno));
        return -1;
    }

    for (r = res; r != NULL ; r = r->ai_next) {
        fd = socket (r->ai_family, r->ai_socktype, r->ai_protocol);
        if (fd == -1) {
           continue;
        }
#if 1	 
        rc = connect (fd, r->ai_addr, r->ai_addrlen);
        if (rc == -1) {
        if (errno == EINPROGRESS) {
            break;
        } else {
            MSF_NETWORK_LOG(DBG_ERROR, "Connect errno:%s.", strerror(errno));
            sclose(fd);
            continue;
        }

    }
#else  //server
	 ret = bind(sock->fd, paddr, sock->addr_len);
	 ret = listen(sock->fd, BACKLOG);
#endif
        break; /* success */
    }

    freeaddrinfo(res);

    if (unlikely(NULL == r)) {
        goto err;
    }

    if (unlikely(msf_socket_nonblocking(fd) < 0 || msf_socket_tcp_nodelay(fd) < 0
         || msf_socket_linger(fd) < 0 || msf_socket_alive(fd) < 0)) {
     printf ("socket_set_noblocking,linger,alive,tcp_nodelay: %s", strerror (errno));
     goto err;
    }

#if 0
    if (socket_reuseport(fd) < 0) {
        MSF_NETWORK_LOG(DBG_ERROR, "socket_nonblocking failed.");
        goto err;
    }


    if (socket_reuseaddr(fd) < 0) {
        MSF_NETWORK_LOG(DBG_ERROR, "socket_nonblocking failed.");
        goto err;
    }

    if(socket_bind(fd, r->ai_addr, r->ai_addrlen, 128) < 0) {
        MSF_NETWORK_LOG(DBG_ERROR, "Cannot listen on %s:%s.", host, port);
        goto err;
    }
#endif
    return fd;

 err:
     sclose(fd);
     return -1;
 }


s32 msf_sendmsg(s32 clifd, struct msghdr *msg) {

    s32 rc = -1;

    rc = sendmsg(clifd, msg, MSG_NOSIGNAL | MSG_WAITALL);
    return rc;
}

s32 msf_recvmsg(s32 clifd, struct msghdr *msg) {

    s32 rc = -1;

    rc = recvmsg(clifd, msg, MSG_NOSIGNAL | MSG_WAITALL);
    return rc;
}

 s32 udp_sendn(const s32 fd, const void * const buf_, 
                 u32 count, const u32 timeout)
  {
     struct sockaddr_in sa;
     sa.sin_family = AF_INET;
     sa.sin_addr.s_addr = inet_addr("192.168.0.110");
     sa.sin_port = htons(9999);
     int ret = sendto(fd, buf_, count, 0, (struct sockaddr*)&sa, sizeof(sa));
#if 0
 bytes_sent = sendto(to->fd, data, len, 0,
     SOCK_PADDR(to), to->addr_len);
#endif
     return ret;
 }
 
/* Sets the DSCP value of socket 'fd' to 'dscp', which must be 63 or less.
* 'family' must indicate the socket's address family (AF_INET or AF_INET6, to
* do anything useful). */
s32 msf_set_dscp(s32 fd, u32 family, u8 dscp) {
    s32 retval;
    s32 val;

#ifdef _WIN32
    /* XXX: Consider using QoS2 APIs for Windows to set dscp. */
    return 0;
#endif

    if (dscp > 63) {
      return -1;
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
          return ENOPROTOOPT;
    }

    return retval ? errno : 0;
}

/* Reads and discards up to 'n' datagrams from 'fd', stopping as soon as no
* more data can be immediately read.	('fd' should therefore be in
* non-blocking mode.)*/
void msf_drain_fd(s32 fd, u32 n_packets) {

    if (invalid_socket == fd) return;

    for (; n_packets > 0; n_packets--) {
        /* 'buffer' only needs to be 1 byte long in most circumstances.  This
        * size is defensive against the possibility that we someday want to
        * use a Linux tap device without TUN_NO_PI, in which case a buffer
        * smaller than sizeof(struct tun_pi) will give EINVAL on read. */
        s8 buffer[128];
        if (read(fd, buffer, sizeof buffer) <= 0) {
            break;
        }
    }
}
 
 
 /*
  * Sets a socket's send buffer size to the maximum allowed by the system.
  */
void msf_socket_maximize_sndbuf(const s32 sfd) {
    socklen_t intsize = sizeof(s32);
    s32 last_good = 0;
    s32 min, max, avg;
    s32 old_size;

#define MAX_SENDBUF_SIZE (256 * 1024 * 1024) //16 * 1024

    /* Start with the default size. */
    if (getsockopt(sfd, SOL_SOCKET, SO_SNDBUF, &old_size, &intsize) != 0) {
        perror("getsockopt(SO_SNDBUF)");
        return;
    }

    /* Binary-search for the real maximum. */
    min = old_size;
    max = MAX_SENDBUF_SIZE;

    while (min <= max) {
     avg = ((u32)(min + max)) / 2;
     /*
      * On Unix domain sockets
      *   Linux uses 224K on both send and receive directions;
      *   FreeBSD, MacOSX, NetBSD, and OpenBSD use 2K buffer size
      *   on send direction and 4K buffer size on receive direction;
      *   Solaris uses 16K on send direction and 5K on receive direction.
      */
     if (setsockopt(sfd, SOL_SOCKET, SO_SNDBUF, (void *)&avg, intsize) == 0) {
    	 last_good = avg;
    	 min = avg + 1;
     } else {
    	 max = avg - 1;
     }
    }
    MSF_NETWORK_LOG(DBG_ERROR, "<%d send buffer was %d, now %d.", sfd, old_size, last_good);
}

/*	
地址结构体分析
sockaddr和sockaddr_in在字节长度上都为16个BYTE,可以进行转换
struct   sockaddr   {  
    unsigned   short    sa_family;    	 //2 
    char                sa_data[14];     //14
};
struct   sockaddr_in   {  
    short  int              sin_family;   //2
    unsigned   short int    sin_port;     //2
    struct  in_addr         sin_addr;     //4  32位IP地址
    unsigned   char         sin_zero[8];  //8
};  
struct   in_addr   {  
    union {
    struct { u_char s_b1,s_b2,s_b3,s_b4; } S_un_b;
    struct { u_short s_w1,s_w2; } S_un_w;
    u_long S_addr; 
} S_un;
#define s_addr  S_un.S_addr
};  
或者
struct in_addr {
in_addr_t s_addr;
//unsigned long s_addr;
};
结构体in_addr 用来表示一个32位的IPv4地址
inet_addr()是将一个点分制的IP地址(如192.168.0.1)转换
为上述结构中需要的32位二进制方式的IP地址(0xC0A80001).
server_addr.sin_addr.s_addr=htonl(INADDR_ANY); 
struct sockaddr_un
struct ifaddrs 
{ 
    struct ifaddrs *ifa_next;     // Pointer to the next structure.   
    char *ifa_name;               // Name of this network interface.  
    unsigned int ifa_flags;       // Flags as from SIOCGIFFLAGS ioctl.   
    struct sockaddr *ifa_addr;    // Network address of this interface.  
    struct sockaddr *ifa_netmask; // Netmask of this interface.  
    union 
    { 
    //At most one of the following two is valid.  If the IFF_BROADCAST 
    // bit is set in `ifa_flags', then `ifa_broadaddr' is valid.  If the 
    //IFF_POINTOPOINT bit is set, then `ifa_dstaddr' is valid. 
    // It is never the case that both these bits are set at once. 
    struct sockaddr *ifu_broadaddr; //Broadcast address of this interface. 
    struct sockaddr *ifu_dstaddr; 	// Point-to-point destination address.  
} ifa_ifu; 
//ese very same macros are defined by <net/if.h> for `struct ifaddr'. 
//So if they are defined already, the existing definitions will be fine.  
# ifndef ifa_broadaddr 
#  define ifa_broadaddr ifa_ifu.ifu_broadaddr 
# endif 
# ifndef ifa_dstaddr 
#  define ifa_dstaddr   ifa_ifu.ifu_dstaddr 
# endif 
void *ifa_data;     //Address-specific data (may be unused).   
}; 
*/

//Linux下编程获取本地IP地址的常见方法

/* 方法一:ioctl()获取本地IP地址 
Linux 下 可以使用ioctl()函数以及结构体 
struct ifreq和结构体struct ifconf来获取网络接口的各种信息。
具体过程是先通过ictol获取本地的所有接口信息,存放到ifconf结构中
再从其中取出每个ifreq表示的ip信息（一般每个网卡对应一个IP地址,
如：”eth0…、eth1…”)。
先了解结构体 struct ifreq和结构体struct ifconf:
ifconf通常是用来保存所有接口信息的
头文件if.h
struct ifconf 
{
    int    ifc_len;    // size of buffer 
    union 
    {
        char *ifcu_buf;  //input from user->kernel
        struct ifreq *ifcu_req; // return from kernel->user
    } ifc_ifcu;
};
#define ifc_buf ifc_ifcu.ifcu_buf //buffer address 
#define ifc_req ifc_ifcu.ifcu_req //array of structures
//ifreq用来保存某个接口的信息
//if.h
struct ifreq {
    char ifr_name[IFNAMSIZ];
    union {
    struct sockaddr ifru_addr;
    struct sockaddr ifru_dstaddr;
    struct sockaddr ifru_broadaddr;
    short ifru_flags;
    int ifru_metric;
    caddr_t ifru_data;
} ifr_ifru;
};
#define ifr_addr ifr_ifru.ifru_addr
#define ifr_dstaddr ifr_ifru.ifru_dstaddr
#define ifr_broadaddr ifr_ifru.ifru_broadaddr
*/

s32 get_ifaddr_by_ioconf(s8 *iface, s8 *ip, s32 len)
{
    s32             sock_fd = -1;
    s32             i = 0;
    struct in_addr  addr_temp;
    struct ifconf   ifconf;
    struct ifreq*   ifreq = NULL;
    unsigned char   buf[512];


    bzero(buf, sizeof(buf));
    bzero(&addr_temp, sizeof(addr_temp));
    bzero(&ifconf, sizeof(ifconf));

    if (!iface || !ip || len < 16) {
        MSF_NETWORK_LOG(DBG_ERROR, "get_ipaddr param err .");
        return -1;
    }

    if((sock_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        MSF_NETWORK_LOG(DBG_ERROR, "socket failed !errno = %d.", errno);
        return -1;
    }

    ifconf.ifc_len = 512;
    ifconf.ifc_buf = (char*)buf;

    /*
    * Get all interfaces list
    */
    if(ioctl(sock_fd, SIOCGIFCONF, &ifconf) < 0) {
        MSF_NETWORK_LOG(DBG_ERROR, "SIOCGIFCONF socket failed !errno = %d.", errno);
        sclose(sock_fd);
        return -1;
    }

    sclose(sock_fd);

    ifreq = (struct ifreq*)ifconf.ifc_buf;
    for(i = (ifconf.ifc_len/sizeof(struct ifreq)); i > 0; i--)
    {
        if(AF_INET == ifreq->ifr_flags   //for ipv4
        &&  0 == msf_strncmp(ifreq->ifr_name, iface, sizeof(ifreq->ifr_name)))
        {
            MSF_NETWORK_LOG(DBG_ERROR, "Interface_name:[%s], IP_addr:[%s], ifr_size:[%lu].", 
            ifreq->ifr_name, 
            inet_ntoa(((struct sockaddr_in*)&(ifreq->ifr_addr))->sin_addr), 
            sizeof(struct ifreq));
            break;
        }
        ifreq++;
    }

    if(0 == i)
    return -1;

    addr_temp = ((struct sockaddr_in*)&(ifreq->ifr_addr))->sin_addr;

    /* 原先使用 snprintf(p_ip, len, "%s", inet_ntoa(addr_temp));
    * 由于inet_ntoa的返回值是共用一片内存，多线程操作返回值有概率会错乱掉
    * 因此用inet_ntop取代inet_ntoa函数
    */
    (void)inet_ntop(AF_INET, &addr_temp, ip, len);

    return 0;
}

s32 get_ipaddr_by_intf(s8* iface, s8* ipaddr, s32 len){
    s32 sock = -1;  
    s8  ip[32];  
    struct ifreq ifr;  

    sock = socket(AF_INET, SOCK_DGRAM, 0); 
    if(sock < 0) {
        return -1;
    }
    strcpy(ifr.ifr_name, iface);  
    ioctl(sock, SIOCGIFADDR, &ifr);  
    strcpy(ip, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));  

    //return string(ip);

    return 0;
}
/* 方法二：getsockname()获取本地IP地址 
如果建立TCP,UDP连接的情况下
(UDP无法获取对端的信息,而且TCP情况下获取地址属性family注意,
要看本地绑定的协议族是啥,有些IPV4和IPV6都绑定的是AF_INT6 -hikvision)
可以通过getsockname和getpeername函数来获取本地和对端的IP和端口号
*/
int get_ifaddr_by_sock(char* ip, int slen) {

    int     fd = -1;
    socklen_t len = sizeof(struct sockaddr_in);
    //sockaddr_storage �㨹����v6o��v4��??��
    struct sockaddr_in servaddr;
    struct sockaddr_in localaddr;
    struct sockaddr_in peeraddr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0) {
        return -1;
    }

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family =   AF_INET;
    servaddr.sin_port   =   htons(9000);

    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // inet_addr("192.168.0.1");   

    if (connect(fd,(struct sockaddr*)&servaddr,sizeof(servaddr)) < 0 ) {
        return -1;
    }
    s8 buf[30];
    memset(buf, 0, sizeof(buf));

    bzero(&localaddr,sizeof(localaddr));
    getsockname(fd, (struct sockaddr*)&localaddr, &len);
    MSF_NETWORK_LOG(DBG_ERROR, "local ip is %s port:%d.", 
    inet_ntop(AF_INET, &localaddr.sin_addr, buf, sizeof(buf)),
    ntohs(localaddr.sin_port));
    bzero(&peeraddr,sizeof(peeraddr));
    getpeername(fd, (struct sockaddr*)&peeraddr, &len);

    MSF_NETWORK_LOG(DBG_ERROR, "peer ip is %s port:%d.", 
    inet_ntop(AF_INET, &peeraddr.sin_addr, buf, sizeof(buf)),
    ntohs(peeraddr.sin_port));

    return 0;
}


/* 方法三：getaddrinfo()获取本地IP地址 
注意:getaddrinfo()可以完成网络主机中主机名和服务名到地址的映射,
但是一般不能用来获取本地IP地址,当它用来获取本地IP地址时,
返回的一般是127.0.0.1本地回环地址。 
*/

s32 msf_get_hostname(s8 *host, s32 len) {
    return gethostname(host, len);
}

s32 msf_get_sockaddr_by_host(const s8 *host, struct sockaddr *addr) {

    s32 rc = -1;
    struct addrinfo* ailist=NULL, *aip=NULL;
    struct sockaddr_in* saddr;

    if (!host || !addr) {
        MSF_NETWORK_LOG(DBG_ERROR, "Input param host or adde is invalid.");
        return -1;
    }

    //getaddrinfo("www.luotang.me", "http", NULL, &ailist);
    rc = getaddrinfo(host, NULL,NULL, &ailist);
    for(aip = ailist; aip != NULL; aip = aip->ai_next) {
        if(aip->ai_family == AF_INET || aip->ai_family == AF_INET6) {
           memcpy(addr, aip->ai_addr, sizeof(struct sockaddr));
           return aip->ai_family;
        }
    }
    return -1;
}

s32 msf_get_ipaddr_by_host(const s8 *host, s8 *ip, s32 len) {

    s32 rc = -1;
    struct sockaddr_in *sin = NULL;
    struct sockaddr_in6 *sin6 = NULL;
    struct sockaddr addr;
    memset(&addr, 0, sizeof(addr));
    rc = msf_get_sockaddr_by_host(host, &addr);
    if (rc < 0) {
        return -1;
    }

    if (rc == AF_INET) {
        sin = SIN(&addr);
        /* inet_ntoa(sin->sin_addr) */
        inet_ntop(AF_INET, &sin->sin_addr, ip, len);
        MSF_NETWORK_LOG(DBG_DEBUG, "Network host ipv4 [%s] port[%d].", ip, ntohs(sin->sin_port));

    } else if (rc== AF_INET6) {
        sin6 = SIN6(&addr);
        inet_ntop(AF_INET6, &sin6->sin6_addr, ip, len);
        MSF_NETWORK_LOG(DBG_DEBUG, "Network host ipv6 [%s] port[%d].",  ip, ntohs(sin6->sin6_port));
    }
    return 0;
}

/*
方法四：gethostname()获取本地IP地址 
gethostname()和getaddrinfo()的功能类似,
一般用于通过主机名或者服务名,比如域名来获取主机的IP地址.
但是要想获取本地IP地址的时候,一般获取的是回环地址127.0.0.1
注意,主机的地址是一个列表的形式,原因是当一个主机有多个网络接口时,
及多块网卡或者一个网卡绑定多个IP地址时,自然就有多个IP地址.
以上代码获取的是根据主机名称得到的第一个IP地址.
*/
int get_ifaddr_by_hostname(char* ip, int len) {

    char hostname[128];
    struct hostent *host_ent;
    char* first_ip = NULL;

    gethostname(hostname, sizeof(hostname));
    host_ent = gethostbyname(hostname);
    first_ip = inet_ntoa(*(struct in_addr*)(host_ent->h_addr_list[0]));
    memcpy(ip, first_ip, 16);	
    return 0;
}

/*
方法五：getifaddrs()获取本地IP地址
查找到系统所有的网络接口的信息,包括以太网卡接口和回环接口等
glibc的源码,它是利用netlink来实现的,
所以在使用这个接口的时候一定要确保你的内核支持netlink.
*/
int  get_ifaddr_by_getifaddrs(char* ip, int len) {

    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];
    char netmask[16];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return  -1;
    }

    /* Walk through linked list, maintaining head pointer so we
    *              can free list later */

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == NULL)
    continue;

    family = ifa->ifa_addr->sa_family;

    /* Display interface name and family (including symbolic
    *                  form of the latter for the common families) */

    MSF_NETWORK_LOG(DBG_ERROR, "%s  address family: %d%s.",
    ifa->ifa_name, family,
    (family == AF_PACKET) ? " (AF_PACKET)" :
    (family == AF_INET) ?  " (AF_INET)" :
    (family == AF_INET6) ?  " (AF_INET6)" : "");

    if (NULL != (*ifa).ifa_addr) { 
        inet_ntop(AF_INET, &(((struct sockaddr_in*)((*ifa).ifa_addr))->sin_addr), ip, 64); 
        MSF_NETWORK_LOG(DBG_ERROR, "\t%s", ip); 
    } else { 
        MSF_NETWORK_LOG(DBG_ERROR, "\t\t"); 
    }

    if (NULL != (*ifa).ifa_netmask) { 
        inet_ntop(AF_INET,
        &(((struct sockaddr_in*)((*ifa).ifa_netmask))->sin_addr), netmask, 64); 
            MSF_NETWORK_LOG(DBG_ERROR, "\t%s", netmask); 
        } else { 
            MSF_NETWORK_LOG(DBG_ERROR, "\t\t"); 
    } 
    /* For an AF_INET* interface address, display the address */

    if (family == AF_INET || family == AF_INET6) {
    s = getnameinfo(ifa->ifa_addr,
    (family == AF_INET) ? sizeof(struct sockaddr_in) :
    sizeof(struct sockaddr_in6),
    host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
    if (s != 0) {
        MSF_NETWORK_LOG(DBG_ERROR, "getnameinfo() failed: %s.", gai_strerror(s));
        exit(EXIT_FAILURE);
    }
    MSF_NETWORK_LOG(DBG_ERROR, "\taddress: <%s>.", host);
    }
    }

    freeifaddrs(ifaddr);
    return 0;
}


/*
* Glibc signalfd() wrapper always has the flags argument.	Glibc 2.7
* and 2.8 signalfd() wrappers call the original signalfd() syscall
* without the flags argument.	Glibc 2.9+ signalfd() wrapper at first
* tries to call signalfd4() syscall and if it fails then calls the
* original signalfd() syscall.  For this reason the non-blocking mode
* is set separately.
*/

s32 epoll_add_signal(void) {

    sigset_t  sigmask;
    if (sigprocmask(SIG_BLOCK, &sigmask, NULL) != 0) { 
    return -1;
    }


    s32 fd = signalfd(-1, &sigmask, 0);
    return 0;
}

/*
* Glibc eventfd() wrapper always has the flags argument.	Glibc 2.7
* and 2.8 eventfd() wrappers call the original eventfd() syscall
* without the flags argument.  Glibc 2.9+ eventfd() wrapper at first
* tries to call eventfd2() syscall and if it fails then calls the
* original eventfd() syscall.  For this reason the non-blocking mode
* is set separately.
*/

/*
* The maximum value after write() to a eventfd() descriptor will
* block or return EAGAIN is 0xFFFFFFFFFFFFFFFE, so the descriptor
* can be read once per many notifications, for example, once per
* 2^32-2 noticifcations.	Since the eventfd() file descriptor is
* always registered in EPOLLET mode, epoll returns event about
* only the latest write() to the descriptor.
*/	

/* create a UNIX domain stream socket */
s32 msf_server_unix_socket(s32 backlog, s8 *unixpath, s32 access_mask) {

    s32 sfd = -1;
    u32 len = 0;
    struct stat tstat;
    mode_t   mode;
    s32 old_umask;
    struct sockaddr_un s_un;

    memset(&s_un, 0, sizeof(struct sockaddr_un));
    s_un.sun_family = AF_UNIX;

    sfd = msf_socket_create(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (unlikely(sfd < 0)) {
        MSF_NETWORK_LOG(DBG_ERROR, "socket failed errno(%s).", strerror(errno));
        return -1;
    }

    if (unlikely(msf_socket_reuseaddr(sfd) < 0)) {
        MSF_NETWORK_LOG(DBG_ERROR, "set socket opt  errno: %s.", strerror(errno));
        sclose(sfd);
        return -1;
    }

    if (unlikely(msf_socket_linger(sfd) < 0)) {
        sclose(sfd);
        return -1;
    }

    /* Clean up a previous socket file if we left it around
    */
    if (lstat(unixpath, &tstat) == 0) {
        if (S_ISSOCK(tstat.st_mode))
        unlink(unixpath);/* in case it already exists */
    }

    mode = (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    if (chmod((s8 *) unixpath, mode) == -1) {
    }

    memcpy(s_un.sun_path, unixpath, min(sizeof(s_un.sun_path)-1, strlen(unixpath)));
    len = offsetof(struct sockaddr_un, sun_path) + strlen(s_un.sun_path);

    old_umask = umask(~(access_mask & 0777));

    /* bind the name to the descriptor */
    if (bind(sfd, (struct sockaddr *)&s_un, sizeof(s_un)) < 0) {
        umask(old_umask);
        MSF_NETWORK_LOG(DBG_ERROR, "Bind %s failed  errno: %s.", 
        s_un.sun_path, strerror(errno));
        sclose(sfd);
        return -1;
    }
    umask(old_umask);

#if 0
    if (unlikely(socket_tcp_nodelay(sfd) < 0)) {
        MSF_NETWORK_LOG(DBG_ERROR, "4set socket opt  errno: %s.", strerror(errno));
        sclose(sfd);
        return -1;
    }
#endif

    if (listen(sfd, backlog) < 0) { 
        /* tell kernel we're a server */
        MSF_NETWORK_LOG(DBG_ERROR, "listen call failed errno %s.", strerror(errno));
        sclose(sfd);
        return -1;
    }
    return sfd;
}
 
/**
* Create a socket and bind it to a specific port number
* @param interface the interface to bind to
* @param port the port number to bind to
* @param transport the transport protocol (TCP / UDP)
* @param portnumber_file A filepointer to write the port numbers to
*    when they are successfully added to the list of ports we listen on.
*/
s32 msf_server_socket(s8 *interf, s32 protocol, s32 port, s32 backlog) {

#ifdef _WIN32
    WSADATA wsa_data;
    WSAStartup(0x0201, &wsa_data);
#endif

    u32 len = sizeof(struct sockaddr_in);
    s32 sfd = -1;
    s32 error = -1;
    struct addrinfo *ai;
    struct addrinfo *next;
    struct addrinfo hints = { .ai_flags = AI_PASSIVE,
    				   .ai_family = AF_UNSPEC };
    s8 port_buf[NI_MAXSERV];

    hints.ai_socktype = IS_UDP(protocol) ? SOCK_DGRAM : SOCK_STREAM;

    snprintf(port_buf, sizeof(port_buf), "%d", port);

    error = getaddrinfo(interf, port_buf, &hints, &ai);
    if (unlikely(error != 0)) {
        if (error != EAI_SYSTEM)
            MSF_NETWORK_LOG(DBG_ERROR, "getaddrinfo(): %s.", gai_strerror(error));
        else
            perror("getaddrinfo()");
        return -1;
    }

    for (next= ai; next; next= next->ai_next) {
    if ((sfd = socket(next->ai_family, next->ai_socktype,
         next->ai_protocol)) == -1) {
        /* getaddrinfo can return "junk" addresses,
        * we make sure at least one works before erroring.
        */
        if (errno == EMFILE) {
        /* ...unless we're out of fds */
        perror("server_socket");
        goto err;
        }
        continue;
    }

    /* set socket attribute */

#ifdef IPV6_V6ONLY
    s32 flags = 1;

    if (next->ai_family == AF_INET6) {
     error = setsockopt(sfd, IPPROTO_IPV6, IPV6_V6ONLY, 
                (char *) &flags, sizeof(flags));
     if (error != 0) {
        perror("setsockopt");
        sclose(sfd);
        continue;
     }
    }
#endif

    msf_socket_nonblocking(sfd);
    msf_socket_reuseaddr(sfd);
    if (IS_UDP(protocol)) {
        msf_socket_maximize_sndbuf(sfd);
    } else {
        msf_socket_linger(sfd);
        msf_socket_alive(sfd);
        msf_socket_tcp_nodelay(sfd);
    }
    /* set socket attribute */

    if (bind(sfd, next->ai_addr, next->ai_addrlen) == -1) {
        if (errno != EADDRINUSE) {
        perror("bind()");
        goto err;
        }
        sclose(sfd);
        continue;
    } else {
        if (!IS_UDP(protocol) && listen(sfd, backlog) < 0) {
        perror("listen()");
        goto err;
        }
        if (next->ai_addr->sa_family == AF_INET ||
         next->ai_addr->sa_family == AF_INET6) {

         struct sockaddr_storage saddr;
         memset(&saddr, 0, sizeof(saddr));

        socklen_t len = sizeof(saddr);

        struct sockaddr_in* sin = (struct sockaddr_in*)&saddr;	

        if (getsockname(sfd, (struct sockaddr*)&saddr, &len) == 0) {
            if (next->ai_addr->sa_family == AF_INET) {
                MSF_NETWORK_LOG(DBG_ERROR, "[%s] fd[%d] %s INET: %u.", 
                    inet_ntoa(sin->sin_addr),
                    sfd,
                    IS_UDP(protocol) ? "UDP" : "TCP",
                    ntohs(sin->sin_port));
            } else if (next->ai_addr->sa_family == AF_INET6){
                MSF_NETWORK_LOG(DBG_ERROR, "[%s] fd[%d] %s INET6: %u.", 
                    inet_ntoa(sin->sin_addr),
                    sfd,
                    IS_UDP(protocol) ? "UDP" : "TCP",
                    ntohs(sin->sin_port));
            } else if (next->ai_addr->sa_family == AF_UNIX) {
              struct sockaddr_un* cun = (struct sockaddr_un*)&saddr;

              MSF_NETWORK_LOG(DBG_ERROR, "server unix_path[%s].", cun->sun_path);
            }
        }
        }
       }

    }

    freeaddrinfo(ai);

    return sfd;

err:
    freeaddrinfo(ai);
    sclose(sfd);
    return -1;
}


//http://www.cnblogs.com/Anker/archive/2013/08/17/3263780.html
/* http://blog.csdn.net/yusiguyuan/article/details/15027821
EPOLLET:
将EPOLL设为边缘触发(Edge Triggered)模式,
这是相对于水平触发(Level Triggered)来说的
EPOLL事件有两种模型：
Edge Triggered(ET)	 
高速工作方式,错误率比较大,只支持no_block socket (非阻塞socket)
LevelTriggered(LT)	 
缺省工作方式，即默认的工作方式,支持blocksocket和no_blocksocket,错误率比较小.
EPOLLIN:
listen fd,有新连接请求,对端发送普通数据
EPOLLPRI:
有紧急的数据可读(这里应该表示有带外数据到来)
EPOLLERR:
表示对应的文件描述符发生错误
EPOLLHUP:
表示对应的文件描述符被挂断 
对端正常关闭(程序里close(),shell下kill或ctr+c),触发EPOLLIN和EPOLLRDHUP,
但是不触发EPOLLERR 和EPOLLHUP.再man epoll_ctl看下后两个事件的说明,
这两个应该是本端（server端）出错才触发的.
EPOLLRDHUP:
这个好像有些系统检测不到,可以使用EPOLLIN,read返回0,删除掉事件,关闭close(fd)
EPOLLONESHOT:
只监听一次事件,当监听完这次事件之后,
如果还需要继续监听这个socket的话,
需要再次把这个socket加入到EPOLL队列里
对端异常断开连接(只测了拔网线),没触发任何事件。
epoll的优点：
1.支持一个进程打开大数目的socket描述符(FD)
select 最不能忍受的是一个进程所打开的FD是有一定限制的，
由FD_SETSIZE设置，默认值是2048。
对于那些需要支持的上万连接数目的IM服务器来说显然太少了。
这时候你一是可以选择修改这个宏然后重新编译内核，
不过资料也同时指出这样会带来网络效率的下降，
二是可以选择多进程的解决方案(传统的 Apache方案)，
不过虽然linux上面创建进程的代价比较小，但仍旧是不可忽视的，
加上进程间数据同步远比不上线程间同步的高效，所以也不是一种完美的方案。
不过 epoll则没有这个限制，它所支持的FD上限是最大可以打开文件的数目，
这个数字一般远大于2048,举个例子,在1GB内存的机器上大约是10万左右，
具体数目可以cat /proc/sys/fs/file-max察看,一般来说这个数目和系统内存关系很大。
2.IO效率不随FD数目增加而线性下降
传统的select/poll另一个致命弱点就是当你拥有一个很大的socket集合，
不过由于网络延时，任一时间只有部分的socket是"活跃"的，
但是select/poll每次调用都会线性扫描全部的集合，导致效率呈现线性下降。
但是epoll不存在这个问题，它只会对"活跃"的socket进行操作---
这是因为在内核实现中epoll是根据每个fd上面的callback函数实现的。
那么，只有"活跃"的socket才会主动的去调用 callback函数，
其他idle状态socket则不会，在这点上，epoll实现了一个"伪"AIO，
因为这时候推动力在os内核。在一些 benchmark中，
如果所有的socket基本上都是活跃的---比如一个高速LAN环境，
epoll并不比select/poll有什么效率，相反，如果过多使用epoll_ctl,
效率相比还有稍微的下降。但是一旦使用idle connections模拟WAN环境,
epoll的效率就远在select/poll之上了
3.使用mmap加速内核与用户空间的消息传递
这点实际上涉及到epoll的具体实现了。
无论是select,poll还是epoll都需要内核把FD消息通知给用户空间，
如何避免不必要的内存拷贝就很重要，在这点上，
epoll是通过内核于用户空间mmap同一块内存实现的。
而如果你想我一样从2.5内核就关注epoll的话，一定不会忘记手工 mmap这一步的。
4.内核微调
这一点其实不算epoll的优点了，而是整个linux平台的优点。
也许你可以怀疑linux平台，但是你无法回避linux平台赋予你微调内核的能力。
比如，内核TCP/IP协议栈使用内存池管理sk_buff结构，
那么可以在运行时期动态调整这个内存pool(skb_head_pool)的大小--- 
通过echoXXXX>/proc/sys/net/core/hot_list_length完成。
再比如listen函数的第2个参数(TCP完成3次握手的数据包队列长度)，
也可以根据你平台内存大小动态调整。
更甚至在一个数据包面数目巨大但同时每个数据包本身大小却很小的特殊系统上
尝试最新的NAPI网卡驱动架构。
*/


int get_gateway(char *if_name, char *p_gateway, int len)
{
    char devname[64];
    unsigned long d = 0;
    unsigned long g = 0;
    unsigned long m = 0;
    int r = 0;
    int flgs = 0;
    int ref = 0;
    int use = 0;
    int metric = 0;
    int mtu = 0;
    int win = 0;
    int ir = 0;
    struct in_addr mask,dst,gw;
    int ret = -1;
    FILE *fp = NULL;

    memset(devname, 0, sizeof(devname));
    memset(&mask, 0, sizeof(mask));
    memset(&dst, 0, sizeof(dst));
    memset(&gw, 0, sizeof(gw));

    if (!if_name || !p_gateway || len < 16){
        return -1;
    }

    fp = fopen(ROUTE_FILE, "r");
    if(NULL == fp){
        MSF_NETWORK_LOG(DBG_ERROR, "open file failed %d.", errno);
        return -1;
    }

    if (fscanf(fp, "%*[^\n]\n") < 0)
    { /* Skip the first line. */
        fclose(fp);
        return -1; 	   /* Empty or missing line, or read error. */
    }

    memset(&mask, 0, sizeof(struct in_addr));
    memset(&dst, 0, sizeof(struct in_addr));
    memset(&gw, 0, sizeof(struct in_addr));

    while (1)
    {
        r = fscanf(fp, "%63s%lx%lx%X%d%d%d%lx%d%d%d\n",
         devname, &d, &g, &flgs, &ref, &use, &metric, &m, &mtu, &win, &ir);
        if (r != 11){
            MSF_NETWORK_LOG(DBG_ERROR, "get_gateway fscanf error and r=%d,errno=%d.", 
            r, errno);
            break;
        }

        if (!(flgs & RTF_UP)){ /* Skip interfaces that are down. */
         continue;
        }

        if (strcmp(devname, if_name) != 0){
         continue;
        }

        mask.s_addr = m;
        dst.s_addr  = d;
        gw.s_addr	 = g;

        if(flgs & RTF_GATEWAY){
            if ( (0 == d) && (0 == m) && (0 != g)){
                /* ?-?����1��? snprintf(p_gateway, len, "%s", inet_ntoa(gw));;
                * ��������inet_ntoa��?����???�̨�?12��?��????����?��??��??3��2�����¡���???�̨�D???��?���䨪?����?
                * ������?��?inet_ntop��?�䨲inet_ntoao����y
                */
                inet_ntop(AF_INET, &gw, p_gateway, len);
                ret = 0;
                break;
            }
        }
    }

    fclose(fp);

    return ret;
}

int set_socket_mcast_ttl(int fd, int optval)
{
    return setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&optval, sizeof(optval));
}

int BaseNet_GetSubNet(char* subnet, char* ifname){

    int     fd_netmask;
    char    netmask_addr[50];

    struct ifreq ifr_mask;
    struct sockaddr_in *net_mask;

    fd_netmask = socket( AF_INET, SOCK_STREAM, 0 );
    if(fd_netmask == -1)
    {
        perror("create socket failture...GetLocalNetMask.");
        return -1;
    }

    memset(&ifr_mask, 0, sizeof(ifr_mask));
    strncpy(ifr_mask.ifr_name, ifname, sizeof(ifr_mask.ifr_name )-1);

    if( (ioctl(fd_netmask, SIOCGIFNETMASK, &ifr_mask ) ) < 0 )
    {
        MSF_NETWORK_LOG(DBG_ERROR, "mac ioctl error.");
        return -1;
    }

    net_mask = ( struct sockaddr_in * )&( ifr_mask.ifr_netmask );
    strcpy( netmask_addr, inet_ntoa( net_mask -> sin_addr ) );

    MSF_NETWORK_LOG(DBG_ERROR, "local netmask:%s.",netmask_addr);


    sclose(fd_netmask);

    return 0;
}
int BaseNet_SetSubNet(char* subnet){
    int fd_netmask;
    char netmask_addr[32];

    struct ifreq ifr_mask;
    struct sockaddr_in *sin_net_mask;

    fd_netmask = socket( AF_INET, SOCK_STREAM, 0 );
    if( fd_netmask == -1) {
        perror("Not create network socket connect.");
        return -1;
    }

    memset(&ifr_mask, 0, sizeof(ifr_mask));
    strncpy(ifr_mask.ifr_name, "eth0", sizeof(ifr_mask.ifr_name )-1);
    sin_net_mask = (struct sockaddr_in *)&ifr_mask.ifr_addr;
    sin_net_mask -> sin_family = AF_INET;
    inet_pton(AF_INET, subnet, &sin_net_mask ->sin_addr);

    if(ioctl(fd_netmask, SIOCSIFNETMASK, &ifr_mask ) < 0) {
        MSF_NETWORK_LOG(DBG_ERROR, "sock_netmask ioctl error.");
        return -1;
    }

    return 0;
}

s32 if_up(s8 *if_name)
{
    int s = -1;
    struct ifreq ifr;
    short flag = 0;

    memset(&ifr, 0, sizeof(ifr));

    if(!if_name){
        return -1;
    }

    if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        return -1;
    }

    strncpy(ifr.ifr_name, if_name, MIN(strlen(if_name), 16));

    flag = IFF_UP;

    if(ioctl(s, SIOCGIFFLAGS, &ifr) < 0){
        MSF_NETWORK_LOG(DBG_ERROR, "if_up ioctl(SIOCGIFFLAGS) error and errno=%d.", errno);
        sclose(s);
        return -1;
    }

    ifr.ifr_ifru.ifru_flags |= flag;

    if(ioctl(s, SIOCSIFFLAGS, &ifr) < 0){
        MSF_NETWORK_LOG(DBG_ERROR, "if_up ioctl(SIOCSIFFLAGS) error and errno=%d.", errno);
        sclose(s);
        return -1;
    }

    sclose(s);
    return 0;
}

s32 if_down(s8* if_name)
{
    s32 s = -1;
    struct ifreq ifr;
    short flag = 0;

    memset(&ifr, 0, sizeof(ifr));

    if (!if_name){
        return -1;
    }

    if (0 == msf_strcmp(if_name, "lo")){
        MSF_NETWORK_LOG(DBG_ERROR, "You can't pull down interface lo.");
        return -1;
    }

    if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        return -1;
    }

    memcpy(ifr.ifr_name, if_name, min(strlen(if_name), sizeof(ifr.ifr_name)-1));

    flag = ~(IFF_UP);

    if (ioctl(s, SIOCGIFFLAGS, &ifr) < 0){
        MSF_NETWORK_LOG(DBG_ERROR, "if_down ioctl error and errno=%d.", errno);
        sclose(s);
        return -1;
    }

    ifr.ifr_ifru.ifru_flags &= flag;

    if(ioctl(s, SIOCSIFFLAGS, &ifr) < 0){
        MSF_NETWORK_LOG(DBG_ERROR, "if_down ioctl error and errno=%d.", errno);
        sclose(s);
        return -1;
    }
    sclose(s);
    return 0;
}


int get_mac(char* if_name, char* p_mac, int len)
{
    int s = -1;
    struct ifreq ifr;
    unsigned char* ptr = NULL;

    memset(&ifr, 0, sizeof(struct ifreq));

    if (!if_name || !p_mac){
        MSF_NETWORK_LOG(DBG_ERROR, "get_mac param err.");
        return -1;
    }

    if(0 == strcmp(if_name, "lo")){
        return -1;
    }

    if(len < 18){
        MSF_NETWORK_LOG(DBG_ERROR, "The mac need 18 byte !.");
        return -1;
    }

    if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        MSF_NETWORK_LOG(DBG_ERROR, "socket failed! errno = %d.", errno);
        return -1;
    }

    strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name)-1);

    if(ioctl(s, SIOCGIFHWADDR, &ifr) != 0){
        MSF_NETWORK_LOG(DBG_ERROR, "get_mac ioctl error and errno=%d.", errno);
        sclose(s);
        return -1;
    }

    ptr =(unsigned char*)&ifr.ifr_ifru.ifru_hwaddr.sa_data[0]; 

    snprintf(p_mac, len, "%02x-%02x-%02x-%02x-%02x-%02x", 
    *ptr, *(ptr+1), *(ptr+2), *(ptr+3), *(ptr+4), *(ptr+5));

    sclose(s);

return 0;
}

int set_mac(char* if_name, char* p_mac_addr)
{
    int s = -1;
    struct ifreq ifr;
    sa_family_t get_family = 0;
    short tmp = 0;
    int i = 0;
    int j = 0;

    memset(&ifr, 0, sizeof(ifr));

    if(!if_name || !p_mac_addr) {
        MSF_NETWORK_LOG(DBG_ERROR, "param error.");
        return -1;
    }

    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        return -1;
    }

    memcpy(ifr.ifr_name, if_name, min(strlen(if_name), sizeof(ifr.ifr_name)-1));

    if (ioctl(s, SIOCGIFHWADDR, &ifr) < 0){
        MSF_NETWORK_LOG(DBG_ERROR, "ioctl error and errno=%d.",errno);
        sclose(s);
        return -1;
    }

    get_family = ifr.ifr_ifru.ifru_hwaddr.sa_family;

    if(if_down(if_name) != 0){
        MSF_NETWORK_LOG(DBG_ERROR, "if_down failed.");
    }

    bzero(&ifr, sizeof(struct ifreq));
    memcpy(ifr.ifr_name, if_name, min(strlen(if_name), sizeof(ifr.ifr_name)-1));
    ifr.ifr_ifru.ifru_hwaddr.sa_family = get_family;

    j = 0;
    for(i = 0;i < 17; i += 3)
    {
        if(p_mac_addr[i] < 58 && p_mac_addr[i] > 47){
        tmp = p_mac_addr[i]-48;
        }
        if(p_mac_addr[i] < 71 && p_mac_addr[i] > 64){
        tmp = p_mac_addr[i]-55;
        }
        if(p_mac_addr[i] < 103 && p_mac_addr[i] > 96){
        tmp = p_mac_addr[i]-87;
        }

        tmp = tmp << 4;
        if(p_mac_addr[i+1] < 58 && p_mac_addr[i+1] > 47){
        tmp |= (p_mac_addr[i+1]-48);
        }
        if(p_mac_addr[i+1] < 71 && p_mac_addr[i+1] > 64){
        tmp |= (p_mac_addr[i+1]-55);
        }
        if(p_mac_addr[i+1] < 103 && p_mac_addr[i+1] > 96){
        tmp |= (p_mac_addr[i+1]-87);
        }
        memcpy(&ifr.ifr_ifru.ifru_hwaddr.sa_data[j++],&tmp,1);
    }

    if(ioctl(s, SIOCSIFHWADDR, &ifr) < 0){
        MSF_NETWORK_LOG(DBG_ERROR, "set_mac ioctl error and errno=%d.", errno);
        sclose(s);
        return -1;
    }

    if(if_up(if_name) != 0){
        MSF_NETWORK_LOG(DBG_ERROR, "if_up down failed.");
    }
    sclose(s);
    return 0;
}

int get_mac_num(char* if_name, char* p_mac, int len) {
    int s = -1;
    struct ifreq ifr;
    unsigned char* ptr = NULL;

    memset(&ifr, 0, sizeof(ifr));

    if (!if_name || !p_mac){
        MSF_NETWORK_LOG(DBG_ERROR, "get_mac_num param error.");
        return -1;
    }

    if(strcmp(if_name, "lo") == 0){
        return -1;
    }

    if(len < 6){
        MSF_NETWORK_LOG(DBG_ERROR, "The mac need 18 byte !.");
        return -1;
    }

    if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        perror("Socket");
        return -1;
    }

    strncpy(ifr.ifr_name, if_name, MIN(strlen(if_name), 16));

    if(ioctl(s, SIOCGIFHWADDR, &ifr) != 0){
        MSF_NETWORK_LOG(DBG_ERROR, "get_mac_num ioctl error and errno=%d.", errno);
        sclose(s);
        return -1;
    }

    ptr =(unsigned char*)&ifr.ifr_ifru.ifru_hwaddr.sa_data[0]; 
    memcpy(p_mac, ptr, 6);
    sclose(s);

    return 0;
}

int getMacNum(void){

    int nCount = 0;
    FILE* f = fopen("/proc/net/dev", "r");
    if (!f)
    {
        MSF_NETWORK_LOG(DBG_ERROR, "Open /proc/net/dev failed!errno:%d.", errno);
        return nCount;
    }

    char szLine[512];

    fgets(szLine, sizeof(szLine), f);	  /* eat line */
    fgets(szLine, sizeof(szLine), f);

    while(fgets(szLine, sizeof(szLine), f)){
    char szName[128] = {0};
    sscanf(szLine, "%s", szName);
    int nLen = strlen(szName);
    if (nLen <= 0)continue;
    if (szName[nLen - 1] == ':') szName[nLen - 1] = 0;
    if (strcmp(szName, "lo") == 0)continue;
        nCount++;
    }

    sfclose(f);
    f = NULL;
    return nCount;
}

int get_netmask(char* if_name, char *p_netmask, int len)
{
    int  s = -1;
    struct ifreq ifr;
    struct sockaddr_in *ptr = NULL;
    struct in_addr addr_temp;

    memset(&ifr, 0, sizeof(ifr));
    memset(&addr_temp, 0, sizeof(addr_temp));

    if (!if_name || !p_netmask){
        MSF_NETWORK_LOG(DBG_ERROR, "get_netmask param error.");
        return -1;
    }

    if(len < 16){
        MSF_NETWORK_LOG(DBG_ERROR, "The netmask need 16 byte !.");
        return -1;
    }

    if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        MSF_NETWORK_LOG(DBG_ERROR, "socket failed !errno = %d.", errno);
        return -1;
    }

    strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name)-1);

    if(ioctl(s, SIOCGIFNETMASK, &ifr) < 0){
        MSF_NETWORK_LOG(DBG_ERROR, "get_netmask ioctl error and errno=%d.", errno);
        sclose(s);
        return -1;
    }

    ptr = (struct sockaddr_in *)&ifr.ifr_ifru.ifru_netmask;
    addr_temp = ptr->sin_addr;  
    /* ?-?����1��? snprintf(p_netmask, len, "%s", inet_ntoa(addr_temp));
    * ��������inet_ntoa��?����???�̨�?12��?��????����?��??��??3��2�����¡���???�̨�D???��?���䨪?����?
    * ������?��?inet_ntop��?�䨲inet_ntoao����y
    */
    (void)inet_ntop(AF_INET, &addr_temp, p_netmask, len);
    sclose(s);

    return 0;
}

int set_netmask(char* if_name, char* p_netmask)
{
    int s = -1;
    struct ifreq ifr;
    struct sockaddr_in netmask_addr;

    memset(&ifr, 0, sizeof(ifr));
    memset(&netmask_addr, 0, sizeof(netmask_addr));

    if (!if_name || !p_netmask){
        MSF_NETWORK_LOG(DBG_ERROR, "set_netmask: param err.");
        return -1;
    }

    if((s = socket(PF_INET,SOCK_STREAM,0)) < 0){
        return -1;
    }

    memcpy(ifr.ifr_name, if_name, min(strlen(if_name), sizeof(ifr.ifr_name)-1));

    bzero(&netmask_addr, sizeof(struct sockaddr_in));
    netmask_addr.sin_family = PF_INET;
    (void)inet_aton(p_netmask, &netmask_addr.sin_addr);

    memcpy(&ifr.ifr_ifru.ifru_netmask, &netmask_addr, sizeof(struct sockaddr_in));

    if(ioctl(s, SIOCSIFNETMASK, &ifr) < 0){
        MSF_NETWORK_LOG(DBG_ERROR, "set_netmask ioctl error and errno=%d.", errno);
        sclose(s);
        return -1;
    }
    sclose(s);
    return 0;
}

int get_broadcast_addr(char* if_name, char *p_broadcast, int len)
{
    int s = -1;
    struct ifreq ifr;
    struct sockaddr_in* ptr = NULL;
    struct in_addr addr_temp;

    memset(&ifr, 0, sizeof(ifr));
    memset(&addr_temp, 0, sizeof(addr_temp));

    if (!if_name || !p_broadcast){
        MSF_NETWORK_LOG(DBG_ERROR, "get_broadcast_addr param error.");
        return -1;
    }

    if(len<16){
        MSF_NETWORK_LOG(DBG_ERROR, "The broadcast need 16 byte !.");
        return -1;
    }

    if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        MSF_NETWORK_LOG(DBG_ERROR, "socket failed !errno = %d.", errno);
        return -1;
    }

    strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name)-1);

    if(ioctl(s, SIOCGIFBRDADDR, &ifr) < 0){
        MSF_NETWORK_LOG(DBG_ERROR, "get_broadcast ioctl error and errno=%d.", errno);
        sclose(s);
        return -1;
    }

    ptr = (struct sockaddr_in *)&ifr.ifr_ifru.ifru_broadaddr;
    addr_temp = ptr->sin_addr;
    (void)inet_ntop(AF_INET, &addr_temp, p_broadcast, len);

    sclose(s);

    return 0;
}
 
int set_broadcast_addr(char* if_name, char* p_broadcast)
{
    int s = -1;
    struct ifreq ifr;
    struct sockaddr_in broadcast_addr;

    memset(&ifr, 0, sizeof(ifr));
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));

    if (!if_name || !p_broadcast){
        return -1;
    }

    if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        return -1;
    }

    memcpy(ifr.ifr_name, if_name, min(strlen(if_name), sizeof(ifr.ifr_name)-1));
    bzero(&broadcast_addr, sizeof(struct sockaddr_in));
    broadcast_addr.sin_family = PF_INET;
    (void)inet_aton(p_broadcast, &broadcast_addr.sin_addr);

    memcpy(&ifr.ifr_ifru.ifru_broadaddr, &broadcast_addr, sizeof(struct sockaddr_in));

    if(ioctl(s, SIOCSIFBRDADDR, &ifr) < 0){
        sclose(s);
        MSF_NETWORK_LOG(DBG_ERROR, "set_broadcast_addr ioctl error and errno=%d.", errno);
        return -1;
    }

    sclose(s);

return 0;
}
 
int add_ipv6_addr(char *p_ip_v6, int prefix_len)
{
    struct in6_ifreq ifr6;
    struct ifreq ifr;
    int socketfd = -1;
    int ret_val = 0;

    memset(&ifr6, 0, sizeof(ifr6));
    memset(&ifr, 0, sizeof(ifr));

    /*?���o3��?��o?����?��?a3~127*/
    if((prefix_len < 3) || (prefix_len > 127) || (NULL == p_ip_v6)){
        return -1;
    }

    if((socketfd = msf_socket_create(AF_INET6, SOCK_DGRAM, 0)) < 0){
        return -1;
    }
    strncpy(ifr.ifr_name, "eth0", sizeof(ifr.ifr_name));
    if(ioctl(socketfd, SIOCGIFINDEX, (char*)&ifr) < 0){
        sclose(socketfd);
        return -1;
    }

    ret_val = inet_pton(AF_INET6, p_ip_v6, &ifr6.ifr6_addr);
    if(ret_val != 0){
        sclose(socketfd);
        return -1;
    }

    ifr6.ifr6_prefixlen = prefix_len;
    ifr6.ifr6_ifindex = ifr.ifr_ifindex;
    if(ioctl(socketfd, SIOCSIFADDR, &ifr6) < 0){;
     sclose(socketfd);
     return -1;
    }

    sclose(socketfd);

    return 0;
}
 

int del_ipv6_addr(char* p_ip_v6, int prefix_len)
{
    struct ifreq ifr;
    struct in6_ifreq ifr6;
    int socketfd = -1;
    int ret_val = 0;

    if(!p_ip_v6){
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    memset(&ifr6, 0, sizeof(ifr6));

    if((socketfd = msf_socket_create(AF_INET6, SOCK_DGRAM, 0)) < 0){
        return -1;
    }

    strncpy(ifr.ifr_name, "eth0", sizeof(ifr.ifr_name));
    if(ioctl(socketfd, SIOCGIFINDEX, (caddr_t) &ifr) < 0){
        sclose(socketfd);
        return -1;
    }

    ret_val = inet_pton(AF_INET6, p_ip_v6, &ifr6.ifr6_addr);
    if(ret_val != 0)	 {
        sclose(socketfd);
        return -1;
    }

    ifr6.ifr6_ifindex = ifr.ifr_ifindex;
    ifr6.ifr6_prefixlen = prefix_len;
    if(ioctl(socketfd, SIOCDIFADDR, &ifr6) < 0){
     if(errno != EADDRNOTAVAIL){
        if(!(errno == EIO))
        MSF_NETWORK_LOG(DBG_ERROR, "del_ipv6_addr: ioctl(SIOCDIFADDR).");
     }else{
        MSF_NETWORK_LOG(DBG_ERROR, "del_ipv6_addr: ioctl(SIOCDIFADDR): No such address, errno %d.", errno);
     }

     sclose(socketfd);
     return -1;
    }

    sclose(socketfd);
    return 0;
}

int del_v6_gateway(struct in6_addr *ipaddr)
{
    int sockfd = -1;
    struct in6_rtmsg v6_rt;
    struct sockaddr_in6 gateway_addr;	 

    memset(&v6_rt, 0, sizeof(v6_rt));
    memset(&gateway_addr, 0, sizeof(gateway_addr));

    if (!ipaddr){
        return -1;
    }

    if ((sockfd = socket(PF_INET6, SOCK_DGRAM, 0)) < 0){		 
        return -1;  
    }
    memset(&v6_rt, 0, sizeof(v6_rt));
    memset(&gateway_addr, 0, sizeof(gateway_addr));
    memcpy(&gateway_addr.sin6_addr, &g_any6addr, sizeof(g_any6addr));
    gateway_addr.sin6_family = PF_INET6;
    memcpy(&v6_rt.rtmsg_gateway, &gateway_addr.sin6_addr, sizeof(struct in6_addr));
    memcpy(&v6_rt.rtmsg_dst, ipaddr, sizeof(struct in6_addr));
    v6_rt.rtmsg_flags &= ~RTF_UP;
    v6_rt.rtmsg_dst_len = 0;
    v6_rt.rtmsg_metric = 0;
    /*��?3y?������*/
    if (ioctl(sockfd, SIOCDELRT, &v6_rt) < 0) {
        sclose(sockfd); 
        MSF_NETWORK_LOG(DBG_ERROR, "del route ioctl error and errno=%d.", errno);
        return -1;  
    }
    sclose(sockfd); 

    return 0;
}
 
int set_v6gateway(char* if_name, char* gateway)
{
    int sockfd = -1;
    struct in6_rtmsg v6_rt;
    struct in6_addr valid_ipv6;
    struct ifreq ifr;

    memset(&v6_rt, 0, sizeof(v6_rt));
    memset(&valid_ipv6, 0, sizeof(valid_ipv6));
    memset(&ifr, 0, sizeof(ifr));

    if (!if_name|| !gateway){		 
        return -1;
    }

    /*DT???a��?1?��???������??��?a��?��?��??��,��??��?����???��y?Y???����??????����?��???������?*/
    memcpy(&valid_ipv6, &g_any6addr, sizeof(valid_ipv6));
    /*��?3y??IPv6��??����??����D��?1?*/
    while(del_v6_gateway(&valid_ipv6) == 0);

    if ((sockfd = socket(PF_INET6, SOCK_DGRAM, 0)) < 0){		 
        return -1;  
    }

    memset(&v6_rt, 0, sizeof(struct in6_rtmsg));

    memcpy(&v6_rt.rtmsg_dst, &valid_ipv6, sizeof(struct in6_addr));
    v6_rt.rtmsg_flags = RTF_UP | RTF_GATEWAY;

    /*??������??��?��??3��?��?a0��??��??������??��?a?����DIP*/
    v6_rt.rtmsg_dst_len = 0;
    v6_rt.rtmsg_metric = 1;

    memset(&ifr, 0, sizeof(ifr));
    /*��???������???:'eth0'*/
    memcpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name));
    (void)ioctl(sockfd, SIOGIFINDEX, &ifr);
    v6_rt.rtmsg_ifindex = ifr.ifr_ifindex;

    memcpy(&v6_rt.rtmsg_gateway, gateway, sizeof(struct in6_addr));
    /*����?��?������*/
    if (ioctl(sockfd, SIOCADDRT, &v6_rt) < 0){
        sclose(sockfd);
        MSF_NETWORK_LOG(DBG_ERROR, "add route ioctl error and errno=%d.", errno);
        return -1;
    }
    sclose(sockfd);

    return 0;
}
 

int get_valid_ipv6(char *device_name, struct in6_addr *ipv6, unsigned int *prefix)
{
    struct in6_addr ipv61;
    struct in6_addr ipv62;
    struct in6_addr inv_addr2;  
    unsigned int prefix1 = 0;
    unsigned int prefix2 = 0;

    memset(&ipv61, 0, sizeof(ipv61));
    memset(&ipv62, 0, sizeof(ipv62));
    memset(&inv_addr2, 0, sizeof(inv_addr2));

    if (!device_name || !ipv6|| !prefix) {
        return -1;
    }
    memset(&inv_addr2, 0, sizeof(struct in6_addr)); 
    inv_addr2.s6_addr[14] = 0x10;/*0:0:0:0:0:0:0:1 ?��??��??��*/

#if 0
    if(get_ipv6addr_num(device_name, &ipv61, &prefix1, &ipv62, &prefix2) != OK)
    {
    MSF_NETWORK_LOG(DBG_ERROR, "update_net_param-- get_ipv6addr_num failed,errno:%d.",errno);  
    }
    else
    {
    if(!IN6_IS_ADDR_UNSPECIFIED(&ipv61))		 
    {
     if (IN6_IS_ADDR_LINKLOCAL(&ipv61) || IN6_IS_ADDR_LOOPBACK(&ipv61))  /*IPv61?TD��*/  
     {
        if (!IN6_IS_ADDR_UNSPECIFIED(&ipv62) && !IN6_IS_ADDR_LINKLOCAL(&ipv62) && !IN6_IS_ADDR_LOOPBACK(&ipv62))
        {
            // IPv62��?��DD���??��
            memcpy(ipv6, &ipv62, sizeof(ipv62));
            *prefix = prefix2;
             
            return 0;
        }

        /*IPv61?��IPv62???TD��*/		 
        memcpy(ipv6, &g_any6addr, sizeof(g_any6addr));

        return 0;
    {
    else
    {
         // IPv61��?��DD���??��
         memcpy(ipv6, &ipv61, sizeof(ipv61));
         *prefix = prefix1;
         
         return 0;
    }
    }
    else
    {
     memcpy(ipv6, &g_any6addr, sizeof(g_any6addr));
     
     return 0;
    }
    }
#endif
    return -1;
}
 
s32 msf_get_dns(s8 *primary_dns, s8 *second_dns) {
    s8    buf[128], name[50];
    s8*   dns[2];
    s32   dns_index = 0;
    FILE* fp = NULL;

    memset(buf, 0, sizeof(buf));
    memset(name, 0, sizeof(name));
    memset(dns, 0, sizeof(dns));

    if (!primary_dns || !second_dns){
        MSF_NETWORK_LOG(DBG_ERROR, "get_dns: param err.");
        return -1;
    }

    fp = fopen(RESOLV_FILE, "r");
    if(NULL == fp){
        MSF_NETWORK_LOG(DBG_ERROR, "can not open file /etc/resolv.conf.");
        return -1;
    }

    dns[0] = primary_dns;
    dns[1] = second_dns;

    while(fgets(buf, sizeof(buf), fp)) {
        sscanf(buf, "%s", name);
        if(!strcmp(name, "nameserver")) {
            sscanf(buf, "%s%s", name, dns[dns_index++]);
        }

        if (dns_index >= 2){
            break;
        }
    }

    fclose(fp);

    return 0;
}

int set_mtu(char *if_name, int mtu)
{
    int s = -1;
    struct ifreq ifr;
    struct sockaddr_in broadcast_addr;

    memset(&ifr, 0, sizeof(ifr));
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));

    if(!if_name){
        return -1;
    }

    if(mtu > 1500 || mtu < 100){
        mtu = 1500;
    }

    if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        return -1;
    }
    strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name)-1);

    bzero(&broadcast_addr, sizeof(struct sockaddr_in));
    broadcast_addr.sin_family = PF_INET;

    ifr.ifr_ifru.ifru_mtu = mtu;

    if(ioctl(s, SIOCSIFMTU, &ifr) < 0){
        sclose(s);
        MSF_NETWORK_LOG(DBG_ERROR, "set_mtu ioctl error and errno=%d.", errno);
        return -1;
    }
    sclose(s);

    return 0;
}
 
int set_net_if_param(char *ifname, int speed, 
         int duplex, int autoneg)
{
    int fd = -1;
    int err = 0;
    struct ifreq ifr;
    struct ethtool_cmd ecmd;

    memset(&ifr, 0, sizeof(ifr));
    memset(&ecmd, 0, sizeof(ecmd));

    if(!ifname){
        MSF_NETWORK_LOG(DBG_ERROR, "set_net_param param error .");
        return -1;
    }

    MSF_NETWORK_LOG(DBG_ERROR, "name = %s, speed = %d, duplex = %d, and autoneg = %d.", 
     ifname, speed, duplex, autoneg);

    /* Setup our control structures. */
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, MIN(strlen(ifname), 16));

    /* Open control socket. */
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        MSF_NETWORK_LOG(DBG_ERROR, "Cannot get control socket.");
        return -1;
    }

    ecmd.cmd = ETHTOOL_GSET;

    ifr.ifr_data = (caddr_t)&ecmd;
    err = ioctl(fd, SIOCETHTOOL, &ifr);
    if (err < 0) {
        MSF_NETWORK_LOG(DBG_ERROR, "Cannot get current device settings.");
    }
    else
    {
     if (!autoneg){
        ecmd.autoneg = AUTONEG_DISABLE;
        /* Change everything the user specified. */
        switch (speed){
         case 10:
            ecmd.speed = SPEED_10;
            break;
         case 100:
            ecmd.speed = SPEED_100;
            break;
         case 1000:
            ecmd.speed = SPEED_1000;
            break;
         default:
            MSF_NETWORK_LOG(DBG_ERROR, "invalid speed mode.");
            sclose(fd);
            return -1;
        }

         if (!duplex) {
            ecmd.duplex = DUPLEX_HALF;
         } else if (1 == duplex) {
            ecmd.duplex = DUPLEX_FULL;
         } else {
            MSF_NETWORK_LOG(DBG_ERROR, "invlid duplex mode."); 
         }
     }else{
        ecmd.autoneg = AUTONEG_ENABLE;
        ecmd.advertising = ADVERTISED_1000baseT_Half 
            | ADVERTISED_1000baseT_Full 
            | ADVERTISED_100baseT_Full 
            | ADVERTISED_100baseT_Half 
            | ADVERTISED_10baseT_Full 
            | ADVERTISED_10baseT_Half 
            | ADVERTISED_Pause;
     }

        /* Try to perform the update. */
        ecmd.cmd = ETHTOOL_SSET;
        ifr.ifr_data = (caddr_t)&ecmd;
        err = ioctl(fd, SIOCETHTOOL, &ifr);
        if (err < 0) {
            MSF_NETWORK_LOG(DBG_ERROR, "Cannot set new settings: %s.", strerror(errno));
            sclose(fd);
            return err;
        }
    }
    sclose(fd);

    return 0;
}

s32 set_active_route(s8* p_route, s8* p_mask, 
     s32 ishost, s8* active_inferface) {
    s32 s = -1;
    struct rtentry rt;
    struct sockaddr_in gateway_addr;

    memset((s8*)&rt, 0, sizeof(struct rtentry));
    memset(&gateway_addr, 0, sizeof(gateway_addr));

    if (!p_route || !p_mask || !active_inferface){
        return -1;
    }

    if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0){
        return -1;
    }

    /* Aug29,2008 - xiemq add	RTF_HOST
    * Nov13,2008 - xiemq add ishost parameters
    */
    if (ishost > 0){
        rt.rt_flags = RTF_UP | RTF_HOST;
    }else{
        rt.rt_flags = RTF_UP;
    }

    bzero(&gateway_addr, sizeof(struct sockaddr_in));
    gateway_addr.sin_family = PF_INET;
    (void)inet_aton(p_route, &gateway_addr.sin_addr);
    memcpy(&rt.rt_dst, &gateway_addr, sizeof(struct sockaddr_in));

    bzero(&gateway_addr, sizeof(struct sockaddr_in));
    (void)inet_aton(p_mask, &gateway_addr.sin_addr);
    gateway_addr.sin_family = PF_INET;
    memcpy(&rt.rt_genmask, &gateway_addr, sizeof(struct sockaddr_in));

    rt.rt_dev = active_inferface;

    if (ioctl(s, SIOCADDRT, &rt) < 0){
        MSF_NETWORK_LOG(DBG_ERROR, "set_route ioctl error and errno=%d.", errno);
        sclose(s);
        return -1;
    }
    sclose(s);
    return 0;
}

int mcast_active_join(int fd, char *active_interface,
         struct sockaddr* p_addrs, int len)
{
    struct ip_mreq mreq;
    struct ifreq ifreq;
    memset(&mreq, 0, sizeof(mreq));
    memset(&ifreq, 0, sizeof(ifreq));

    if(! p_addrs || !active_interface || (fd <= 0) || (len <= 0)){
        return -1;
    }

    memcpy(&mreq.imr_multiaddr, &((struct sockaddr_in*)p_addrs)->sin_addr,
         sizeof(struct in_addr));


    strncpy(ifreq.ifr_name, active_interface, sizeof(ifreq.ifr_name)-1);

    // ??��?��???������??��?����?????D??��?��D����??����?
    (void)ioctl(fd, SIOCGIFADDR, &ifreq);
    memcpy(&mreq.imr_interface, &((struct sockaddr_in*)&ifreq.ifr_addr)->sin_addr,
     sizeof(struct in_addr));

    // ??IP?����??��2������
    return setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
}
 
int mcast_active_join_ex(int socket_fd, char *active_interface, 
                 sock_addr_base* p_mcastgroupaddr)
{
    struct sockaddr *psockaddr = NULL;
    struct ip_mreq stru_merq;
    struct ipv6_mreq stru_merq6;
    int  ret_val = -1;
    struct ifreq stru_ifreq;

    if(socket_fd <= 0 || !p_mcastgroupaddr){
        return -1;
    }

    memset(&stru_merq, 0, sizeof(struct ip_mreq));
    memset(&stru_merq6, 0, sizeof(struct ipv6_mreq));
    memset(&stru_ifreq, 0, sizeof(stru_ifreq));
    strncpy(stru_ifreq.ifr_name, active_interface, sizeof(stru_ifreq.ifr_name)-1);

    psockaddr = (struct sockaddr *)&p_mcastgroupaddr;

    if(AF_INET == psockaddr->sa_family) {
        // ??��?��???������??��?����?????D??��?��D����??����?
        (void)ioctl(socket_fd, SIOCGIFADDR, &stru_ifreq);

        stru_merq.imr_multiaddr.s_addr = p_mcastgroupaddr->sa_v4.sin_addr.s_addr;
        stru_merq.imr_interface.s_addr = ((struct sockaddr_in *)&stru_ifreq.ifr_addr)->sin_addr.s_addr;
        ret_val = setsockopt(socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&stru_merq,
         sizeof(stru_merq));
    } else {
     memcpy(&stru_merq6.ipv6mr_multiaddr, &p_mcastgroupaddr->sa_v6.sin6_addr,
             sizeof(struct in6_addr));
     stru_merq6.ipv6mr_interface = if_nametoindex(stru_ifreq.ifr_name);
     ret_val = setsockopt(socket_fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, 
        (char *)&stru_merq6, sizeof(stru_merq6));

    }

    return ret_val;

}

int mcast_leave(int socket_fd, struct sockaddr* p_addrs)
{
    struct ip_mreq mreq;

    memset(&mreq, 0, sizeof(mreq));

    if(!p_addrs){
        return -1;
    }

    memcpy(&mreq.imr_multiaddr, &((struct sockaddr_in*)p_addrs)->sin_addr, 
    sizeof(struct in_addr));

    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    return setsockopt(socket_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, 
         &mreq, sizeof(mreq));
}
 
/**@brief	 ?????��IP�䨮?��2������?D��?3y,??��YIPv6.��?����??OK,������?��?3y3��1|?�ꨨ?����??ERROR,������?��?3y����㨹?��   
* @param[in]	socket_fd ���?����?��??��??����
* @param[in]	p_mcastgroupaddr ?��2��������?��??��
* @param[out] ?T
* @return   OK/ERROR 
*/
s32 mcast_leave_ex(s32 socket_fd, sock_addr_base* p_mcastgroupaddr) {
    struct sockaddr *psockaddr = NULL;
    struct ip_mreq stru_merq;
    struct ipv6_mreq stru_merq6;
    int ret_val = -1;
    struct ifreq stru_ifreq;

    if(socket_fd <= 0 || !p_mcastgroupaddr){
        return -1;
    }

    memset(&stru_merq, 0, sizeof(struct ip_mreq));
    memset(&stru_merq6, 0, sizeof(struct ipv6_mreq));
    memset(&stru_ifreq, 0, sizeof(stru_ifreq));

    strncpy(stru_ifreq.ifr_name, "eth0", IFNAMSIZ);

    psockaddr = (struct sockaddr *)&p_mcastgroupaddr;
    if(AF_INET == psockaddr->sa_family) {
        // ??��?��???������??��?����?????D??��?��D����??����?
        (void)ioctl(socket_fd, SIOCGIFADDR, &stru_ifreq);

        stru_merq.imr_multiaddr.s_addr = p_mcastgroupaddr->sa_v4.sin_addr.s_addr;
        stru_merq.imr_interface.s_addr = ((struct sockaddr_in *)&stru_ifreq.ifr_addr)->sin_addr.s_addr;
        ret_val = setsockopt(socket_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&stru_merq,
            sizeof(stru_merq));
    } else {
        memcpy(&stru_merq6.ipv6mr_multiaddr, &p_mcastgroupaddr->sa_v6.sin6_addr,
         sizeof(struct in6_addr));
        stru_merq6.ipv6mr_interface = if_nametoindex(stru_ifreq.ifr_name);
        ret_val = setsockopt(socket_fd, IPPROTO_IPV6, 
                IPV6_DROP_MEMBERSHIP, (char *)&stru_merq6, sizeof(stru_merq6));
    }

    return ret_val;
}

int mcast_setloop(int socket_fd, int enable)
{
     char flag = 0;
     flag = enable;

    if (socket_fd <= 0){
        return -1;
    }

    return setsockopt(socket_fd, IPPROTO_IP, IP_MULTICAST_LOOP, 
             &flag, sizeof(flag));
}
 
int mcast_setloop_ex(int socket_fd, int enable, short af)
{
    char flag = 0;
    flag = enable;

    if (socket_fd <= 0){
        return -1;
    }

    if(AF_INET == af){
     return setsockopt(socket_fd, IPPROTO_IP, IP_MULTICAST_LOOP,
             &flag, sizeof(flag));
    } else {
     return setsockopt(socket_fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
             &flag, sizeof(flag));
    }
}

int mcast_set_if(int socket_fd, char *active_interface)
{

    int ret_val = -1;
    struct ifreq stru_ifreq;

    if(socket_fd <= 0){
        return -1;
    }

    memset(&stru_ifreq, 0, sizeof(stru_ifreq));
    strncpy(stru_ifreq.ifr_name, active_interface, sizeof(stru_ifreq.ifr_name)-1);


    (void)ioctl(socket_fd, SIOCGIFADDR, &stru_ifreq);
    ret_val = setsockopt(socket_fd, IPPROTO_IP, IP_MULTICAST_IF,
        &((struct sockaddr_in*)&stru_ifreq.ifr_addr)->sin_addr, 
        sizeof(struct in_addr));

    return ret_val;

}

int isSameSubnet(char* if_name, char *gateway)
{
    unsigned int i = 0;
    unsigned int g = 0;
    struct in_addr gw;
    struct in_addr ip;
    struct in_addr mask;
    char tmp[16] = {0};

    if (!if_name || !gateway){
        return -1;
    }

    /* get interface ip address */
    if (get_ipaddr_by_intf(if_name, tmp, sizeof(tmp)) != 0 ){
        return -1;
    }

    memset(&ip, 0, sizeof(struct in_addr));
    if (inet_aton(tmp, &ip) == 0){
        return -1;
    }
    MSF_NETWORK_LOG(DBG_ERROR, "ip address: 0x%x.", ip.s_addr);

    /* get eth0 subnet mask */
    memset(tmp, 0, sizeof(tmp));
    if (get_netmask(if_name, tmp, sizeof(tmp)) != 0){
        return -1;
    }

    memset(&mask, 0, sizeof(struct in_addr));
    if (inet_aton(tmp, &mask) == 0){
        return -1;
    }
    MSF_NETWORK_LOG(DBG_ERROR, "ip mask: 0x%x.", mask.s_addr);

    if(mask.s_addr == 0){
        return -1;
}

/* get gateway */
memset(&gw, 0, sizeof(struct in_addr));
if (inet_aton(gateway, &gw) == 0){
 return -1;
}
MSF_NETWORK_LOG(DBG_ERROR, "ip gateway: 0x%x.", gw.s_addr);

i = (ip.s_addr & mask.s_addr); 		 /* IP & Mask */
g = (gw.s_addr & mask.s_addr); 		 /* Gateway & Mask */
if(i == g){
 MSF_NETWORK_LOG(DBG_ERROR, "is in same subnet.");
 return 1;
}

MSF_NETWORK_LOG(DBG_ERROR, "isn't in same subnet.");
return 0;
}

int del_gateway(void) {
    int err = -1;
    int s = -1;
    struct rtentry rt;
    struct sockaddr_in gateway_addr;

    if((s = socket(PF_INET, SOCK_DGRAM,0)) < 0){
        return -1;
    }

    memset((char *)&rt, 0, sizeof(struct rtentry) );

    bzero(&gateway_addr, sizeof(struct sockaddr_in));
    (void)inet_aton("0.0.0.0", &gateway_addr.sin_addr);
    gateway_addr.sin_family = PF_INET;
    memcpy(&rt.rt_genmask, &gateway_addr, sizeof(struct sockaddr_in)); 
    memcpy(&rt.rt_dst, &gateway_addr, sizeof(struct sockaddr_in));

    if(ioctl(s, SIOCDELRT, &rt) < 0){
        err = errno;
        MSF_NETWORK_LOG(DBG_ERROR, "del_gateway ioctl error and errno=%d.", err);
        sclose(s);
        if(3 == err){
            return 0; // linux general errno 3 :  No such process
        }
        return -1;
    }

    sclose(s);
    return 0;
}

s32 del_active_route(s8* active_if, s8* p_route, s8* p_mask) {
    int s = -1;
    struct rtentry rt;
    struct sockaddr_in gateway_addr;

    memset((char *)&rt, 0, sizeof(rt));
    memset((char *)&gateway_addr, 0, sizeof(gateway_addr));

    if (!active_if || !p_route || !p_mask){
        MSF_NETWORK_LOG(DBG_ERROR, "del_route param error.");
        return -1;
    }

    if((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0){
        MSF_NETWORK_LOG(DBG_ERROR, "del_route create socket error.");
        return -1;
    }

    memset((char *)&rt, 0, sizeof(struct rtentry));
    rt.rt_flags = RTF_UP | RTF_HOST;

    bzero(&gateway_addr, sizeof(struct sockaddr_in));
    gateway_addr.sin_family = PF_INET;
    (void)inet_aton(p_route, &gateway_addr.sin_addr);
    memcpy(&rt.rt_dst, &gateway_addr, sizeof(struct sockaddr_in));

    bzero(&gateway_addr, sizeof(struct sockaddr_in));
    (void)inet_aton(p_mask, &gateway_addr.sin_addr);
    gateway_addr.sin_family = PF_INET;
    memcpy(&rt.rt_genmask, &gateway_addr, sizeof(struct sockaddr_in));

    rt.rt_dev = active_if;

    if(ioctl(s,SIOCDELRT,&rt) < 0){
        MSF_NETWORK_LOG(DBG_ERROR, "del_route ioctl error and errno=%d.",errno);
        sclose(s);
        return -1;
    }
    sclose(s);
    return 0;
}

int set_active_gateway(char* if_name, char* gateway)
{
    int s = -1;
    struct rtentry rt;
    struct sockaddr_in gateway_addr;

    char gwip[16];
    struct in_addr gw;

    memset(&rt, 0, sizeof(rt));
    memset(&gateway_addr, 0, sizeof(gateway_addr));
    memset(gwip, 0, sizeof(gwip));
    memset(&gw, 0, sizeof(gw));

    if (!if_name || !gateway){
        MSF_NETWORK_LOG(DBG_ERROR, "gateway is null !prog return err.");
        return -1;
    }

    if (del_gateway() != 0){
        MSF_NETWORK_LOG(DBG_ERROR, "del_gateway err.");
    }

    memset(gwip, 0, sizeof(gwip));
    /* Aug29,2008 - xiemq add */
    /* delete old gateway */
    if(old_gateway > 0)
    {
        memset(gwip, 0, sizeof(gwip));
        memset(&gw, 0, sizeof(struct in_addr));
        gw.s_addr = old_gateway;
        strncpy(gwip, inet_ntoa(gw), sizeof(gwip)-1);
        (void)del_active_route(if_name, gwip, "0.0.0.0");
    }

    // ����?1��?1?��?����???a0.0.0.0
    if (0 == strcmp(gateway, "0.0.0.0")){
        MSF_NETWORK_LOG(DBG_ERROR, "interface isn't set gateway.");
        return 0;
    }

    if(isSameSubnet(if_name, gateway) == 0)
    {
    /* set new gateway */
    if(set_active_route(gateway, "0.0.0.0", 1, if_name) != 0){
        return -1;
    }

    /* store new gateway */
    memset(&gw, 0, sizeof(struct in_addr));
    (void)inet_aton(gateway, &gw);
    old_gateway = gw.s_addr;
    }

    if((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0){
        return -1;
    }

    memset((char *)&rt,0,sizeof(struct rtentry) );
    rt.rt_flags = RTF_UP | RTF_GATEWAY;

    bzero(&gateway_addr, sizeof(struct sockaddr_in));
    gateway_addr.sin_family = PF_INET;
    (void)inet_aton(gateway, &gateway_addr.sin_addr);
    memcpy(&rt.rt_gateway, &gateway_addr, sizeof(struct sockaddr_in));

    bzero(&gateway_addr, sizeof(struct sockaddr_in));
    (void)inet_aton("0.0.0.0", &gateway_addr.sin_addr);
    gateway_addr.sin_family = PF_INET;
    memcpy(&rt.rt_genmask, &gateway_addr, sizeof(struct sockaddr_in));

    memcpy(&rt.rt_dst, &gateway_addr, sizeof(struct sockaddr_in));

    rt.rt_dev = if_name;

    if(ioctl(s, SIOCADDRT, &rt) < 0){
        MSF_NETWORK_LOG(DBG_ERROR, "set_gateway ioctl error and errno=%d.", errno);
        sclose(s);
        return -1;
    }
    sclose(s);
    return 0;
}

s32 set_ipaddr(const s8* if_name, const s8* ip) {
    s32 s = -1;
    struct ifreq ifr;
    struct sockaddr_in addr;

    memset(&ifr, 0, sizeof(ifr));
    memset(&addr, 0, sizeof(addr));

    if(!if_name || !ip){
        MSF_NETWORK_LOG(DBG_ERROR, "param error.");
        return -1;
    }

    if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        return -1;
    }

    memcpy(ifr.ifr_name, if_name, min(strlen(if_name), sizeof(ifr.ifr_name)-1));

    bzero(&addr, sizeof(struct sockaddr_in));
    addr.sin_family = PF_INET;
    (void)inet_aton(ip, &addr.sin_addr);

    memcpy(&ifr.ifr_ifru.ifru_addr,&addr,sizeof(struct sockaddr_in));

    if(ioctl(s, SIOCSIFADDR, &ifr) < 0){
        MSF_NETWORK_LOG(DBG_ERROR, "set_ipaddr ioctl error and errno=%d.", errno);
        sclose(s);
        return -1;
    }
    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;

    //get the status of the device
    if( ioctl(s, SIOCSIFFLAGS, &ifr ) < 0 ){
         perror("SIOCSIFFLAGS");
         return -1;
    }

    sclose(s);
    return 0;
}


#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/socket.h>
 
#include <msf_network.h>
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
*  2. 使用 netlink 的内核部分可以采用模块的方式实现,使用 netlink 的应用部分和内核部分没有编译时依赖
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
 如果netlink传送消息失败,那么recvmsg()函数会返回No buffer spaceavailable(ENOBUFS)错误
*/

#define NETLINK_TEST    30
#define MSG_LEN         125
#define MAX_PLOAD       125
 
typedef struct _user_msg_info {
    struct nlmsghdr hdr;
    s8  msg[MSG_LEN];
} user_msg_info;
 
s32 netlink_socket_create() {

    s32 nl_fd = -1;
    struct sockaddr_nl saddr;

    /* 第一个参数必须是PF_NETLINK或者AF_NETLINK,
    第二个参数用SOCK_DGRAM和SOCK_RAW都没问题,
    第三个参数就是netlink的协议号*/

    nl_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_TEST);
    if (nl_fd < 0) return -1;

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

    if (bind(nl_fd, (struct sockaddr*)&saddr, sizeof(saddr)) < 0) {
        perror("bind() error.");
        close(nl_fd);
        return -1;
    }

    msf_socket_nonblocking(nl_fd);

    return nl_fd;
}

s32 netlink_sendo(s32 fd, s8 *data, u32 len) {

    struct sockaddr_nl daddr;

    memset(&daddr, 0, sizeof(daddr));
    daddr.nl_family = AF_NETLINK;
    daddr.nl_pad = 0;           /*always set to zero*/
    daddr.nl_pid = 0;           /*kernel's pid is zero*/  
    daddr.nl_groups = 0;        /*multicast groups mask, if unicast set to zero*/  


    struct nlmsghdr *nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PLOAD));
    memset(nlh, 0, sizeof(struct nlmsghdr));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PLOAD);//NLMSG_LENGTH(0);
    nlh->nlmsg_flags = 0;
    nlh->nlmsg_type = 0;
    nlh->nlmsg_seq = 0;
    nlh->nlmsg_pid = getpid();

    memcpy(NLMSG_DATA(nlh), data, len);
    int rc = sendto(fd, nlh, nlh->nlmsg_len, 0, (struct sockaddr *)&daddr, sizeof(struct sockaddr_nl));
    if (!rc) {
        perror("sendto error.");
        sclose(fd);
        exit(-1);
    }
    return rc;
}
 
s32 netlink_recvfrom(s32 fd, s8 *data, u32 len) {

    user_msg_info u_info;
    memset(&u_info, 0, sizeof(u_info));

    struct sockaddr_nl daddr;

    memset(&daddr, 0, sizeof(daddr));
    daddr.nl_family = AF_NETLINK;
    daddr.nl_pid = 0; // to kernel 
    daddr.nl_groups = 0;

    socklen_t sock_len = sizeof(struct sockaddr_nl);
    s32 ret = recvfrom(fd, &u_info, sizeof(user_msg_info), 0, 
    (struct sockaddr *)&daddr, &sock_len);
    if(!ret) {
        perror("recv form kernel error.");
        close(fd);
        return -1;
    }
    return ret;
}
 
 static struct sockaddr_nl src_addr, dest_addr;
 
static s32 nl_write(s32 fd, void *data, s32 len) {
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
    msg.msg_name= (s8*)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = iov;
    msg.msg_iovlen = 2;

    return sendmsg(fd, &msg, 0);
}

static s32 sl_read(s32 fd, void *data, s32 len) {
    struct iovec iov[2];
    struct msghdr msg;
    struct nlmsghdr nlh;

    iov[0].iov_base = &nlh;
    iov[0].iov_len = sizeof(nlh);
    iov[1].iov_base = data;
    iov[1].iov_len = len;

    memset(&msg, 0, sizeof(msg));
    msg.msg_name= (void*)&src_addr;
    msg.msg_namelen = sizeof(src_addr);
    msg.msg_iov = iov;
    msg.msg_iovlen = 2;

    return recvmsg(fd, &msg, MSG_DONTWAIT);
}
 
#if 0
static network_ops netlink_ops = {
    .s_sock_init = netlink_socket_create,
    .s_option_cb = NULL,
    .s_read_cb = netlink_recvfrom,
    .s_write_cb = netlink_sendo,
    .s_drain_cb = NULL,
    .s_close_cb = NULL,
};
#endif

/*
* MSF_EVENT_DISABLED state is used to track whether EPOLLONESHOT
* event should be added or modified, epoll_ctl(2):
*
* EPOLLONESHOT (since Linux 2.6.2)
*     Sets the one-shot behavior for the associated file descriptor.
*     This means that after an event is pulled out with epoll_wait(2)
*     the associated file descriptor is internally disabled and no
*     other events will be reported by the epoll interface.  The user
*     must call epoll_ctl() with EPOLL_CTL_MOD to rearm the file
*     descriptor with a new event mask.
*
* Notice:
*  Although calling close() on a file descriptor will remove any epoll
*  events that reference the descriptor, in this case the close() acquires
*  the kernel global "epmutex" while epoll_ctl(EPOLL_CTL_DEL) does not
*  acquire the "epmutex" since Linux 3.13 if the file descriptor presents
*  only in one epoll set.  Thus removing events explicitly before closing
*  eliminates possible lock contention.
*/
s32 msf_epoll_create(void) {

    s32 ep_fd = -1;

#ifdef EVENT__HAVE_EPOLL_CREATE1
    /* First, try the shiny new epoll_create1 interface, if we have it. */
    ep_fd = epoll_create1(EPOLL_CLOEXEC);
#endif
    if (ep_fd < 0) {
        /* Initialize the kernel queue using the old interface.  
         * (The size field is ignored   since 2.6.8.) */
        ep_fd = epoll_create(512);
        if (ep_fd < 0) {
            if (errno != ENOSYS) {
                MSF_NETWORK_LOG(DBG_ERROR, "Failed to create epoll fd, errno(%d).", errno);
                return -1;
            }
        }
        msf_socket_closeonexec(ep_fd);
    }
    
    return ep_fd;
}

s32 msf_add_event(s32 epfd, s32 clifd, short event, void *p) {
    struct epoll_event ev;
    ev.events = event;
    ev.data.fd = clifd;
    ev.data.ptr = (void*)p;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, clifd, &ev) < 0) {
        /* If an ADD operation fails with EEXIST,
         * either the operation was redundant (as with a
         * precautionary add), or we ran into a fun
         * kernel bug where using dup*() to duplicate the
         * same file into the same fd gives you the same epitem
         * rather than a fresh one.  For the second case,
         * we must retry with MOD. */
        if (errno == EEXIST) {
            if (epoll_ctl(epfd, EPOLL_CTL_MOD, clifd, &ev) < 0) {
                MSF_NETWORK_LOG(DBG_ERROR, "Epoll MOD(%d) on %d retried as ADD; that failed too.",
                     (s32)event, clifd);
                return 0;
            } else {
               MSF_NETWORK_LOG(DBG_ERROR, "Epoll MOD(%d) on %d retried as ADD; succeeded.",
                (s32)event, clifd);
               return -1;
            }
        }
    }
    return 0;
}

s32 msf_mod_event(s32 epfd, s32 clifd, short event) {
    struct epoll_event ev;        
    ev.events = event;          
    ev.data.fd = clifd;     

    if (epoll_ctl(epfd, EPOLL_CTL_MOD, clifd, &ev) < 0) {
        MSF_NETWORK_LOG(DBG_ERROR, "Failed to mod epoll event, errno(%d).", errno);
        /* If a MOD operation fails with ENOENT, the
         * fd was probably closed and re-opened.  We
         * should retry the operation as an ADD.
         */
        if (errno == ENOENT) {
             if (epoll_ctl(epfd, EPOLL_CTL_ADD, clifd, &ev) < 0) {
                MSF_NETWORK_LOG(DBG_ERROR, "Epoll MOD(%d) on %d retried as ADD; that failed too.",
                     (s32)event, clifd);
                return 0;
            } else {
               MSF_NETWORK_LOG(DBG_ERROR, "Epoll MOD(%d) on %d retried as ADD; succeeded.",
                (s32)event, clifd);
               return -1;
            }
        }
    }
    return 0;
}

s32 msf_del_event(s32 epfd, s32 clifd) {

    if (epoll_ctl(epfd, EPOLL_CTL_DEL, clifd, NULL) < 0) {
        MSF_NETWORK_LOG(DBG_ERROR, "Failed to del epoll event, errno(%d).", errno);
        if (errno == ENOENT || errno == EBADF || errno == EPERM) {
            /* If a delete fails with one of these errors,
             * that's fine too: we closed the fd before we
             * got around to calling epoll_dispatch. */
            MSF_NETWORK_LOG(DBG_ERROR, "Epoll DEL on fd %d gave %s: DEL was unnecessary.",
                clifd,
                strerror(errno));
            return 0;
        }
        return -1;
    }
        return 0;
}
