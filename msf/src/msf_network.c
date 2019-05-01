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
 *	 Advantage:	Simple
 *	 Disadvantage: ineffective, but depend on ifconfig or route command.
 * 2.Create Socket,use iocrl().
 *	 Advantage:	effective
 *	 Disadvantage: diffcult.
 *
 *  Get Net Config
 *	1.Use popen(), other side execute ifconfig and route,
 *	  this side recv and parse.
 *	2.Use fopen() to open /proc/net/route, get gateway.
 *	  effective than ifconfig and route command.
 *	3. Create Socket,use iocrl().
*/

/*
1.close()º¯Êı

[cpp] view plaincopyprint?
01.<SPAN style="FONT-SIZE: 13px">#include<unistd.h>  
02.int close(int sockfd);     //·µ»Ø³É¹¦Îª0£¬³ö´íÎª-1.</SPAN>  
#include<unistd.h>
int close(int sockfd);     //·µ»Ø³É¹¦Îª0£¬³ö´íÎª-1.    close Ò»¸öÌ×½Ó×ÖµÄÄ¬ÈÏĞĞÎªÊÇ°ÑÌ×½Ó×Ö±ê¼ÇÎªÒÑ¹Ø±Õ£¬È»ºóÁ¢¼´·µ»Øµ½µ÷ÓÃ½ø³Ì£¬
¸ÃÌ×½Ó×ÖÃèÊö·û²»ÄÜÔÙÓÉµ÷ÓÃ½ø³ÌÊ¹ÓÃ£¬Ò²¾ÍÊÇËµËü²»ÄÜÔÙ×÷Îªread»òwriteµÄµÚÒ»¸ö²ÎÊı£¬È»¶øTCP½«³¢ÊÔ·¢ËÍÒÑÅÅ¶ÓµÈ´ı·¢ËÍµ½¶Ô¶ËµÄÈÎºÎÊı¾İ£¬
·¢ËÍÍê±Ïºó·¢ÉúµÄÊÇÕı³£µÄTCPÁ¬½ÓÖÕÖ¹ĞòÁĞ¡£

    ÔÚ¶à½ø³Ì²¢·¢·şÎñÆ÷ÖĞ£¬¸¸×Ó½ø³Ì¹²Ïí×ÅÌ×½Ó×Ö£¬Ì×½Ó×ÖÃèÊö·ûÒıÓÃ¼ÆÊı¼ÇÂ¼×Å¹²Ïí×ÅµÄ½ø³Ì¸öÊı£¬µ±¸¸½ø³Ì»òÄ³Ò»×Ó½ø³ÌcloseµôÌ×½Ó×ÖÊ±£¬
ÃèÊö·ûÒıÓÃ¼ÆÊı»áÏàÓ¦µÄ¼õÒ»£¬µ±ÒıÓÃ¼ÆÊıÈÔ´óÓÚÁãÊ±£¬Õâ¸öcloseµ÷ÓÃ¾Í²»»áÒı·¢TCPµÄËÄÂ·ÎÕÊÖ¶ÏÁ¬¹ı³Ì¡£

2.shutdown()º¯Êı

[cpp] view plaincopyprint?
01.<SPAN style="FONT-SIZE: 13px">#include<sys/socket.h>  
02.int shutdown(int sockfd,int howto);  //·µ»Ø³É¹¦Îª0£¬³ö´íÎª-1.</SPAN>  
#include<sys/socket.h>
int shutdown(int sockfd,int howto);  //·µ»Ø³É¹¦Îª0£¬³ö´íÎª-1.    ¸Ãº¯ÊıµÄĞĞÎªÒÀÀµÓÚhowtoµÄÖµ

    1.SHUT_RD£ºÖµÎª0£¬¹Ø±ÕÁ¬½ÓµÄ¶ÁÕâÒ»°ë¡£

    2.SHUT_WR£ºÖµÎª1£¬¹Ø±ÕÁ¬½ÓµÄĞ´ÕâÒ»°ë¡£

    3.SHUT_RDWR£ºÖµÎª2£¬Á¬½ÓµÄ¶ÁºÍĞ´¶¼¹Ø±Õ¡£

    ÖÕÖ¹ÍøÂçÁ¬½ÓµÄÍ¨ÓÃ·½·¨ÊÇµ÷ÓÃcloseº¯Êı¡£µ«Ê¹ÓÃshutdownÄÜ¸üºÃµÄ¿ØÖÆ¶ÏÁ¬¹ı³Ì£¨Ê¹ÓÃµÚ¶ş¸ö²ÎÊı£©¡£

3.Á½º¯ÊıµÄÇø±ğ
    closeÓëshutdownµÄÇø±ğÖ÷Òª±íÏÖÔÚ£º
    closeº¯Êı»á¹Ø±ÕÌ×½Ó×ÖID£¬Èç¹ûÓĞÆäËûµÄ½ø³Ì¹²Ïí×ÅÕâ¸öÌ×½Ó×Ö£¬ÄÇÃ´ËüÈÔÈ»ÊÇ´ò¿ªµÄ£¬Õâ¸öÁ¬½ÓÈÔÈ»¿ÉÒÔÓÃÀ´¶ÁºÍĞ´£¬²¢ÇÒÓĞÊ±ºòÕâÊÇ·Ç³£ÖØÒªµÄ £¬ÌØ±ğÊÇ¶ÔÓÚ¶à½ø³Ì²¢·¢·şÎñÆ÷À´Ëµ¡£

    ¶øshutdown»áÇĞ¶Ï½ø³Ì¹²ÏíµÄÌ×½Ó×ÖµÄËùÓĞÁ¬½Ó£¬²»¹ÜÕâ¸öÌ×½Ó×ÖµÄÒıÓÃ¼ÆÊıÊÇ·ñÎªÁã£¬ÄÇĞ©ÊÔÍ¼¶ÁµÃ½ø³Ì½«»á½ÓÊÕµ½EOF±êÊ¶£¬ÄÇĞ©ÊÔÍ¼Ğ´µÄ½ø³Ì½«»á¼ì²âµ½SIGPIPEĞÅºÅ£¬Í¬Ê±¿ÉÀûÓÃshutdownµÄµÚ¶ş¸ö²ÎÊıÑ¡Ôñ¶ÏÁ¬µÄ·½Ê½¡£
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
    log_write(level, MSF_MOD_NETWORK, MSF_FUNC_FILE_LINE, __VA_ARGS__)


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
* linuxÂÊÇtcp_cork,ÉÏÃæµÄÒâË¼¾ÍÊÇËµ,µ±Ê¹ÓÃsendfileº¯ÊıÊ±,
* tcp_nopush²ÅÆğ×÷ÓÃ,ËüºÍÖ¸Áîtcp_nodelayÊÇ»¥³âµÄ
* ÕâÊÇtcp/ip´«ÊäµÄÒ»¸ö±ê×¼ÁË,Õâ¸ö±ê×¼µÄ´ó¸ÅµÄÒâË¼ÊÇ,
* Ò»°ãÇé¿öÏÂ,ÔÚtcp½»»¥µÄ¹ı³ÌÖĞ,µ±Ó¦ÓÃ³ÌĞò½ÓÊÕµ½Êı¾İ°üºóÂíÉÏ´«ËÍ³öÈ¥,²»µÈ´ı,
* ¶øtcp_corkÑ¡ÏîÊÇÊı¾İ°ü²»»áÂíÉÏ´«ËÍ³öÈ¥,µÈµ½Êı¾İ°ü×î´óÊ±,Ò»´ÎĞÔµÄ´«Êä³öÈ¥,
* ÕâÑùÓĞÖúÓÚ½â¾öÍøÂç¶ÂÈû,ÒÑ¾­ÊÇÄ¬ÈÏÁË.
* Ò²¾ÍÊÇËµtcp_nopush = on »áÉèÖÃµ÷ÓÃtcp_cork·½·¨,Õâ¸öÒ²ÊÇÄ¬ÈÏµÄ,
* ½á¹û¾ÍÊÇÊı¾İ°ü²»»áÂíÉÏ´«ËÍ³öÈ¥,µÈµ½Êı¾İ°ü×î´óÊ±,Ò»´ÎĞÔµÄ´«Êä³öÈ¥,
* ÕâÑùÓĞÖúÓÚ½â¾öÍøÂç¶ÂÈû
*
* ÒÔ¿ìµİÍ¶µİ¾ÙÀıËµÃ÷Ò»ÏÂ£¨ÒÔÏÂÊÇÎÒµÄÀí½â,Ò²ĞíÊÇ²»ÕıÈ·µÄ£©£¬µ±¿ìµİ¶«Î÷Ê±,
* ¿ìµİÔ±ÊÕµ½Ò»¸ö°ü¹ü,ÂíÉÏÍ¶µİ,ÕâÑù±£Ö¤ÁË¼´Ê±ĞÔ,µ«ÊÇ»áºÄ·Ñ´óÁ¿µÄÈËÁ¦ÎïÁ¦,
* ÔÚÍøÂçÉÏ±íÏÖ¾ÍÊÇ»áÒıÆğÍøÂç¶ÂÈû,¶øµ±¿ìµİÊÕµ½Ò»¸ö°ü¹ü,°Ñ°ü¹ü·Åµ½¼¯É¢µØ,
* µÈÒ»¶¨ÊıÁ¿ºóÍ³Ò»Í¶µİ,ÕâÑù¾ÍÊÇtcp_corkµÄÑ¡Ïî¸ÉµÄÊÂÇé,ÕâÑùµÄ»°,
* »á×î´ó»¯µÄÀûÓÃÍøÂç×ÊÔ´,ËäÈ»ÓĞÒ»µãµãÑÓ³Ù.
* Õâ¸öÑ¡Ïî¶ÔÓÚwww£¬ftpµÈ´óÎÄ¼şºÜÓĞ°ïÖú
*
* TCP_NODELAYºÍTCP_CORK»ù±¾ÉÏ¿ØÖÆÁË°üµÄ¡°Nagle»¯¡±
* Nagle»¯ÔÚÕâÀïµÄº¬ÒåÊÇ²ÉÓÃNagleËã·¨°Ñ½ÏĞ¡µÄ°ü×é×°Îª¸ü´óµÄÖ¡.
* John NagleÊÇNagleËã·¨µÄ·¢Ã÷ÈË,ºóÕß¾ÍÊÇÓÃËûµÄÃû×ÖÀ´ÃüÃûµÄ,
* ËûÔÚ1984ÄêÊ×´ÎÓÃÕâÖÖ·½·¨À´³¢ÊÔ½â¾ö¸£ÌØÆû³µ¹«Ë¾µÄÍøÂçÓµÈûÎÊÌâ
* (ÓûÁË½âÏêÇéÇë²Î¿´IETF RFC 896).
* Ëû½â¾öµÄÎÊÌâ¾ÍÊÇËùÎ½µÄsilly window syndrome,
* ÖĞÎÄ³Æ¡°ÓŞ´À´°¿ÚÖ¢ºòÈº¡±£¬¾ßÌåº¬ÒåÊÇ,
* ÒòÎªÆÕ±éÖÕ¶ËÓ¦ÓÃ³ÌĞòÃ¿²úÉúÒ»´Î»÷¼ü²Ù×÷¾Í»á·¢ËÍÒ»¸ö°ü,
* ¶øµäĞÍÇé¿öÏÂÒ»¸ö°ü»áÓµÓĞÒ»¸ö×Ö½ÚµÄÊı¾İÔØºÉÒÔ¼°40¸ö×Ö½Ú³¤µÄ°üÍ·,
* ÓÚÊÇ²úÉú4000%µÄ¹ıÔØ,ºÜÇáÒ×µØ¾ÍÄÜÁîÍøÂç·¢ÉúÓµÈû.
* Nagle»¯ºóÀ´³ÉÁËÒ»ÖÖ±ê×¼²¢ÇÒÁ¢¼´ÔÚÒòÌØÍøÉÏµÃÒÔÊµÏÖ.
* ËüÏÖÔÚÒÑ¾­³ÉÎªÈ±Ê¡ÅäÖÃÁË£¬µ«ÔÚÎÒÃÇ¿´À´,
* ÓĞĞ©³¡ºÏÏÂ°ÑÕâÒ»Ñ¡Ïî¹ØµôÒ²ÊÇºÏºõĞèÒªµÄ¡£
*
* ÏÖÔÚÈÃÎÒÃÇ¼ÙÉèÄ³¸öÓ¦ÓÃ³ÌĞò·¢³öÁËÒ»¸öÇëÇó,Ï£Íû·¢ËÍĞ¡¿éÊı¾İ.
* ÎÒÃÇ¿ÉÒÔÑ¡ÔñÁ¢¼´·¢ËÍÊı¾İ»òÕßµÈ´ı²úÉú¸ü¶àµÄÊı¾İÈ»ºóÔÙÒ»´Î·¢ËÍÁ½ÖÖ²ßÂÔ.
* Èç¹ûÎÒÃÇÂíÉÏ·¢ËÍÊı¾İ,ÄÇÃ´½»»¥ĞÔµÄÒÔ¼°¿Í»§/·şÎñÆ÷ĞÍµÄÓ¦ÓÃ³ÌĞò½«¼«´óµØÊÜÒæ.
* Èç¹ûÇëÇóÁ¢¼´·¢³öÄÇÃ´ÏìÓ¦Ê±¼äÒ²»á¿ìÒ»Ğ©.
* ÒÔÉÏ²Ù×÷¿ÉÒÔÍ¨¹ıÉèÖÃÌ×½Ó×ÖµÄTCP_NODELAY = on Ñ¡ÏîÀ´Íê³É,
* ÕâÑù¾Í½ûÓÃÁËNagle Ëã·¨¡£

* ÁíÍâÒ»ÖÖÇé¿öÔòĞèÒªÎÒÃÇµÈµ½Êı¾İÁ¿´ïµ½×î´óÊ±²ÅÍ¨¹ıÍøÂçÒ»´Î·¢ËÍÈ«²¿Êı¾İ,
* ÕâÖÖÊı¾İ´«Êä·½Ê½ÓĞÒæÓÚ´óÁ¿Êı¾İµÄÍ¨ĞÅĞÔÄÜ£¬µäĞÍµÄÓ¦ÓÃ¾ÍÊÇÎÄ¼ş·şÎñÆ÷.
* Ó¦ÓÃ NagleËã·¨ÔÚÕâÖÖÇé¿öÏÂ¾Í»á²úÉúÎÊÌâ,µ«ÊÇ£¬Èç¹ûÄãÕıÔÚ·¢ËÍ´óÁ¿Êı¾İ,
* Äã¿ÉÒÔÉèÖÃTCP_CORKÑ¡Ïî½ûÓÃNagle»¯, Æä·½Ê½ÕıºÃÍ¬ TCP_NODELAYÏà·´
* (TCP_CORKºÍ TCP_NODELAYÊÇ»¥ÏàÅÅ³âµÄ)
* 
* ´ò¿ªsendfileÑ¡ÏîÊ±,È·¶¨¿ªÆôFreeBSDÏµÍ³ÉÏµÄTCP_NOPUSH»òLinuxÏµÍ³ÉÏµÄTCP_CORK¹¦ÄÜ.
* ´ò¿ªtcp_nopushºó, ½«»áÔÚ·¢ËÍÏìÓ¦Ê±°ÑÕû¸öÏìÓ¦°üÍ··Åµ½Ò»¸öTCP°üÖĞ·¢ËÍ.
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

/*
* Ä¬ÈÏÇé¿öÏÂ,serverÖØÆô,µ÷ÓÃsocket,bind,È»ºólisten,»áÊ§°Ü.
* ÒòÎª¸Ã¶Ë¿ÚÕıÔÚ±»Ê¹ÓÃ.Èç¹ûÉè¶¨SO_REUSEADDR,ÄÇÃ´serverÖØÆô²Å»á³É¹¦.
* Òò´Ë,ËùÓĞµÄTCP server¶¼±ØĞëÉè¶¨´ËÑ¡Ïî,ÓÃÒÔÓ¦¶ÔserverÖØÆôµÄÏÖÏó*/
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
NagleËã·¨µÄ»ù±¾¶¨ÒåÊÇÈÎÒâÊ±¿Ì£¬×î¶àÖ»ÄÜÓĞÒ»¸öÎ´±»È·ÈÏµÄĞ¡¶Î¡£ ËùÎ½¡°Ğ¡¶Î¡±£¬Ö¸µÄÊÇĞ¡ÓÚMSS³ß´çµÄÊı¾İ¿é£¬ËùÎ½¡°Î´±»È·ÈÏ¡±£¬ÊÇÖ¸Ò»
¸öÊı¾İ¿é·¢ËÍ³öÈ¥ºó£¬Ã»ÓĞÊÕµ½¶Ô·½·¢ËÍµÄACKÈ·ÈÏ¸ÃÊı¾İÒÑÊÕµ½¡£
NagleËã·¨µÄ¹æÔò£¨¿É²Î¿¼tcp_output.cÎÄ¼şÀïtcp_nagle_checkº¯Êı×¢ÊÍ£©£º
£¨1£©Èç¹û°ü³¤¶È´ïµ½MSS£¬ÔòÔÊĞí·¢ËÍ£»
£¨2£©Èç¹û¸Ã°üº¬ÓĞFIN£¬ÔòÔÊĞí·¢ËÍ£»
£¨3£©ÉèÖÃÁËTCP_NODELAYÑ¡Ïî£¬ÔòÔÊĞí·¢ËÍ£»
£¨4£©Î´ÉèÖÃTCP_CORKÑ¡ÏîÊ±£¬ÈôËùÓĞ·¢³öÈ¥µÄĞ¡Êı¾İ°ü£¨°ü³¤¶ÈĞ¡ÓÚMSS£©¾ù±»È·ÈÏ£¬ÔòÔÊĞí·¢ËÍ£» 
£¨5£©ÉÏÊöÌõ¼ş¶¼Î´Âú×ã£¬µ«·¢ÉúÁË³¬Ê±£¨Ò»°ãÎª200ms£©£¬ÔòÁ¢¼´·¢ËÍ¡£
NagleËã·¨Ö»ÔÊĞíÒ»¸öÎ´±»ACKµÄ°ü´æÔÚÓÚÍøÂç£¬Ëü²¢²»¹Ü°üµÄ´óĞ¡£¬Òò´ËËüÊÂÊµÉÏ¾ÍÊÇÒ»¸öÀ©Õ¹µÄÍ£-µÈĞ­Òé£¬Ö»²»¹ıËüÊÇ»ùÓÚ°üÍ£-µÈµÄ£¬¶ø²»
ÊÇ»ùÓÚ×Ö½ÚÍ£-µÈµÄ¡£NagleËã·¨ÍêÈ«ÓÉTCPĞ­ÒéµÄACK»úÖÆ¾ö¶¨£¬Õâ»á´øÀ´Ò»Ğ©ÎÊÌâ£¬±ÈÈçÈç¹û¶Ô¶ËACK»Ø¸´ºÜ¿ìµÄ»°£¬NagleÊÂÊµÉÏ²»»áÆ´½ÓÌ«¶à
µÄÊı¾İ°ü£¬ËäÈ»±ÜÃâÁËÍøÂçÓµÈû£¬ÍøÂç×ÜÌåµÄÀûÓÃÂÊÒÀÈ»ºÜµÍ¡£


TCPÁ´½ÓµÄ¹ı³ÌÖĞ£¬Ä¬ÈÏ¿ªÆôNagleËã·¨£¬½øĞĞĞ¡°ü·¢ËÍµÄÓÅ»¯¡£
2. TCP_NODELAY Ñ¡Ïî
Ä¬ÈÏÇé¿öÏÂ£¬·¢ËÍÊı¾İ²ÉÓÃNegale Ëã·¨¡£ÕâÑùËäÈ»Ìá¸ßÁËÍøÂçÍÌÍÂÁ¿£¬µ«ÊÇÊµÊ±ĞÔÈ´½µµÍÁË£¬ÔÚÒ»Ğ©½»»¥ĞÔºÜÇ¿µÄÓ¦ÓÃ³ÌĞòÀ´ËµÊÇ²»
ÔÊĞíµÄ£¬Ê¹ÓÃTCP_NODELAYÑ¡Ïî¿ÉÒÔ½ûÖ¹Negale Ëã·¨¡£
´ËÊ±£¬Ó¦ÓÃ³ÌĞòÏòÄÚºËµİ½»µÄÃ¿¸öÊı¾İ°ü¶¼»áÁ¢¼´·¢ËÍ³öÈ¥¡£ĞèÒª×¢ÒâµÄÊÇ£¬ËäÈ»½ûÖ¹ÁËNegale Ëã·¨£¬µ«ÍøÂçµÄ´«ÊäÈÔÈ»ÊÜµ½TCPÈ·ÈÏÑÓ³Ù»úÖÆµÄÓ°Ïì¡£
3. TCP_CORK Ñ¡Ïî (tcp_nopush = on »áÉèÖÃµ÷ÓÃtcp_cork·½·¨£¬ÅäºÏsendfile Ñ¡Ïî½öÔÚÊ¹ÓÃsendfileµÄÊ±ºò²Å¿ªÆô)
ËùÎ½µÄCORK¾ÍÊÇÈû×ÓµÄÒâË¼£¬ĞÎÏóµØÀí½â¾ÍÊÇÓÃCORK½«Á¬½ÓÈû×¡£¬Ê¹µÃÊı¾İÏÈ²»·¢³öÈ¥£¬µÈµ½°ÎÈ¥Èû×ÓºóÔÙ·¢³öÈ¥¡£ÉèÖÃ¸ÃÑ¡Ïîºó£¬ÄÚºË
»á¾¡Á¦°ÑĞ¡Êı¾İ°üÆ´½Ó³ÉÒ»¸ö´óµÄÊı¾İ°ü£¨Ò»¸öMTU£©ÔÙ·¢ËÍ³öÈ¥£¬µ±È»ÈôÒ»¶¨Ê±¼äºó£¨Ò»°ãÎª200ms£¬¸ÃÖµÉĞ´ıÈ·ÈÏ£©£¬ÄÚºËÈÔÈ»Ã»ÓĞ×é
ºÏ³ÉÒ»¸öMTUÊ±Ò²±ØĞë·¢ËÍÏÖÓĞµÄÊı¾İ£¨²»¿ÉÄÜÈÃÊı¾İÒ»Ö±µÈ´ı°É£©¡£
È»¶ø£¬TCP_CORKµÄÊµÏÖ¿ÉÄÜ²¢²»ÏñÄãÏëÏóµÄÄÇÃ´ÍêÃÀ£¬CORK²¢²»»á½«Á¬½ÓÍêÈ«Èû×¡¡£ÄÚºËÆäÊµ²¢²»ÖªµÀÓ¦ÓÃ²ãµ½µ×Ê²Ã´Ê±ºò»á·¢ËÍµÚ¶şÅú
Êı¾İÓÃÓÚºÍµÚÒ»ÅúÊı¾İÆ´½ÓÒÔ´ïµ½MTUµÄ´óĞ¡£¬Òò´ËÄÚºË»á¸ø³öÒ»¸öÊ±¼äÏŞÖÆ£¬ÔÚ¸ÃÊ±¼äÄÚÃ»ÓĞÆ´½Ó³ÉÒ»¸ö´ó°ü£¨Å¬Á¦½Ó½üMTU£©µÄ»°£¬ÄÚ
ºË¾Í»áÎŞÌõ¼ş·¢ËÍ¡£Ò²¾ÍÊÇËµÈôÓ¦ÓÃ²ã³ÌĞò·¢ËÍĞ¡°üÊı¾İµÄ¼ä¸ô²»¹»¶ÌÊ±£¬TCP_CORK¾ÍÃ»ÓĞÒ»µã×÷ÓÃ£¬·´¶øÊ§È¥ÁËÊı¾İµÄÊµÊ±ĞÔ£¨Ã¿¸öĞ¡
°üÊı¾İ¶¼»áÑÓÊ±Ò»¶¨Ê±¼äÔÙ·¢ËÍ£©¡£

4. NagleËã·¨ÓëCORKËã·¨Çø±ğ
NagleËã·¨ºÍCORKËã·¨·Ç³£ÀàËÆ£¬µ«ÊÇËüÃÇµÄ×ÅÑÛµã²»Ò»Ñù£¬NagleËã·¨Ö÷Òª±ÜÃâÍøÂçÒòÎªÌ«¶àµÄĞ¡°ü£¨Ğ­ÒéÍ·µÄ±ÈÀı·Ç³£Ö®´ó£©¶øÓµÈû£¬¶øCORK
Ëã·¨ÔòÊÇÎªÁËÌá¸ßÍøÂçµÄÀûÓÃÂÊ£¬Ê¹µÃ×ÜÌåÉÏĞ­ÒéÍ·Õ¼ÓÃµÄ±ÈÀı¾¡¿ÉÄÜµÄĞ¡¡£Èç´Ë¿´À´Õâ¶şÕßÔÚ±ÜÃâ·¢ËÍĞ¡°üÉÏÊÇÒ»ÖÂµÄ£¬ÔÚÓÃ»§¿ØÖÆµÄ²ãÃæÉÏ£¬
NagleËã·¨ÍêÈ«²»ÊÜÓÃ»§socketµÄ¿ØÖÆ£¬ÄãÖ»ÄÜ¼òµ¥µÄÉèÖÃTCP_NODELAY¶ø½ûÓÃËü£¬CORKËã·¨Í¬ÑùÒ²ÊÇÍ¨¹ıÉèÖÃ»òÕßÇå³ıTCP_CORKÊ¹ÄÜ»òÕß½ûÓÃÖ®£¬
È»¶øNagleËã·¨¹ØĞÄµÄÊÇÍøÂçÓµÈûÎÊÌâ£¬Ö»ÒªËùÓĞµÄACK»ØÀ´Ôò·¢°ü£¬¶øCORKËã·¨È´¿ÉÒÔ¹ØĞÄÄÚÈİ£¬ÔÚÇ°ºóÊı¾İ°ü·¢ËÍ¼ä¸ôºÜ¶ÌµÄÇ°ÌáÏÂ£¨ºÜÖØÒª£¬
·ñÔòÄÚºË»á°ïÄã½«·ÖÉ¢µÄ°ü·¢³ö£©£¬¼´Ê¹ÄãÊÇ·ÖÉ¢·¢ËÍ¶à¸öĞ¡Êı¾İ°ü£¬ÄãÒ²¿ÉÒÔÍ¨¹ıÊ¹ÄÜCORKËã·¨½«ÕâĞ©ÄÚÈİÆ´½ÓÔÚÒ»¸ö°üÄÚ£¬Èç¹û´ËÊ±ÓÃNagle
Ëã·¨µÄ»°£¬Ôò¿ÉÄÜ×ö²»µ½ÕâÒ»µã¡£

naggle(tcp_nodelayÉèÖÃ)Ëã·¨£¬Ö»Òª·¢ËÍ³öÈ¥Ò»¸ö°ü£¬²¢ÇÒÊÜµ½Ğ­ÒéÕ»ACKÓ¦´ğ£¬ÄÚºË¾Í»á¼ÌĞø°Ñ»º³åÇøµÄÊı¾İ·¢ËÍ³öÈ¥¡£
core(tcp_coreÉèÖÃ)Ëã·¨£¬ÊÜµ½¶Ô·½Ó¦´ğºó£¬ÄÚºËÊ×ÏÈ¼ì²éµ±Ç°»º³åÇøÖĞµÄ°üÊÇ·ñÓĞ1500£¬Èç¹ûÓĞÔòÖ±½Ó·¢ËÍ£¬Èç¹ûÊÜµ½Ó¦´ğµÄÊ±ºò»¹Ã»ÓĞ1500£¬Ôò
µÈ´ı200ms£¬Èç¹û200msÄÚ»¹Ã»ÓĞ1500×Ö½Ú£¬Ôò·¢ËÍ
*/ //²Î¿¼http://m.blog.csdn.net/blog/c_cyoxi/8673645

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
       (const void *) &cork, sizeof(int)); //Ñ¡Ïî½öÔÚÊ¹ÓÃsendfileµÄÊ±ºò²Å¿ªÆô
}

int
ngx_tcp_push(s32 s)//Ñ¡Ïî½öÔÚÊ¹ÓÃsendfileµÄÊ±ºò²Å¿ªÆô
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
TCP_DEFER_ACCEPT ÓÅ»¯ Ê¹ÓÃTCP_DEFER_ACCEPT¿ÉÒÔ¼õÉÙÓÃ»§³ÌĞòholdµÄÁ¬½ÓÊı,
Ò²¿ÉÒÔ¼õÉÙÓÃ»§µ÷ÓÃepoll_ctlºÍepoll_waitµÄ´ÎÊı£¬´Ó¶øÌá¸ßÁË³ÌĞòµÄĞÔÄÜ¡£
ÉèÖÃlistenÌ×½Ó×ÖµÄTCP_DEFER_ACCEPTÑ¡Ïîºó£¬ Ö»µ±Ò»¸öÁ´½ÓÓĞÊı¾İÊ±ÊÇ²Å»á
´ÓaccpetÖĞ·µ»Ø£¨¶ø²»ÊÇÈı´ÎÎÕÊÖÍê³É)¡£
ËùÒÔ½ÚÊ¡ÁËÒ»´Î¶ÁµÚÒ»¸öhttpÇëÇó°üµÄ¹ı³Ì,¼õÉÙÁËÏµÍ³µ÷ÓÃ

²éÑ¯×ÊÁÏ£¬TCP_DEFER_ACCEPTÊÇÒ»¸öºÜÓĞÈ¤µÄÑ¡Ïî£¬
Linux Ìá¹©µÄÒ»¸öÌØÊâ setsockopt,¡¡ÔÚ accept µÄ socket ÉÏÃæ,
Ö»ÓĞµ±Êµ¼ÊÊÕµ½ÁËÊı¾İ£¬²Å»½ĞÑÕıÔÚ accept µÄ½ø³Ì£¬¿ÉÒÔ¼õÉÙÒ»Ğ©ÎŞÁÄµÄÉÏÏÂÎÄÇĞ»»¡£´úÂëÈçÏÂ¡£
val = 5;
setsockopt(srv_socket->fd, SOL_TCP, TCP_DEFER_ACCEPT, &val, sizeof(val));
ÀïÃæ val µÄµ¥Î»ÊÇÃë£¬×¢ÒâÈç¹û´ò¿ªÕâ¸ö¹¦ÄÜ£¬kernel ÔÚ val ÃëÖ®ÄÚ»¹Ã»ÓĞ
ÊÕµ½Êı¾İ£¬²»»á¼ÌĞø»½ĞÑ½ø³Ì£¬¶øÊÇÖ±½Ó¶ªÆúÁ¬½Ó¡£
¾­¹ı²âÊÔ·¢ÏÖ£¬ÉèÖÃTCP_DEFER_ACCEPTÑ¡Ïîºó£¬·şÎñÆ÷ÊÜµ½Ò»¸öCONNECTÇëÇóºó£¬
²Ù×÷ÏµÍ³²»»áAccept£¬Ò²²»»á´´½¨IO¾ä±ú¡£²Ù×÷ÏµÍ³Ó¦¸ÃÔÚÈô¸ÉÃë£¬
(µ«¿Ï¶¨Ô¶Ô¶´óÓÚÉÏÃæÉèÖÃµÄ1s) ºó£¬»áÊÍ·ÅÏà¹ØµÄÁ´½Ó¡£
µ«Ã»ÓĞÍ¬Ê±¹Ø±ÕÏàÓ¦µÄ¶Ë¿Ú£¬ËùÒÔ¿Í»§¶Ë»áÒ»Ö±ÒÔÎª´¦ÓÚÁ´½Ó×´Ì¬¡£
Èç¹ûConnectºóÃæÂíÉÏÓĞºóĞøµÄ·¢ËÍÊı¾İ£¬ÄÇÃ´·şÎñÆ÷»áµ÷ÓÃAccept½ÓÊÕÕâ¸öÁ´½Ó¶Ë¿Ú¡£
¸Ğ¾õÁËÒ»ÏÂ£¬Õâ¸ö¶Ë¿ÚÉèÖÃ¶ÔÓÚCONNECTÁ´½ÓÉÏÀ´¶øÓÖÊ²Ã´¶¼²»¸ÉµÄ¹¥»÷·½Ê½´¦ÀíºÜÓĞĞ§¡£
ÎÒÃÇÔ­À´µÄ´úÂë¶¼ÊÇÏÈÔÊĞíÁ´½Ó£¬È»ºóÔÙ½øĞĞ³¬Ê±´¦Àí£¬±ÈËûÕâ¸öÓĞµãOutÁË¡£
²»¹ıÕâ¸öÑ¡Ïî¿ÉÄÜ»áµ¼ÖÂ¶¨Î»Ä³Ğ©ÎÊÌâÂé·³¡£
timeout = 0±íÊ¾È¡Ïû TCP_DEFER_ACCEPTÑ¡Ïî

ĞÔÄÜËÄÉ±ÊÖ£ºÄÚ´æ¿½±´£¬ÄÚ´æ·ÖÅä£¬½ø³ÌÇĞ»»£¬ÏµÍ³µ÷ÓÃ¡£
TCP_DEFER_ACCEPT ¶ÔĞÔÄÜµÄ¹±Ï×£¬¾ÍÔÚÓÚ¼õÉÙÏµÍ³µ÷ÓÃÁË¡£
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

static int parse_numeric_servname(const char *servname)
{
    int n;
    char *endptr=NULL;
    n = (int) strtol(servname, &endptr, 10);
    if (n>=0 && n <= 65535 && servname[0] && endptr && !endptr[0])
        return n;
    else
        return -1;
}

/** Parse a service name in 'servname', which can be a decimal port.
* Return the port number, or -1 on error.
*/
static int msf_parse_servname(const char *servname, const char *protocol,
const struct addrinfo *hints)
{
    int n = parse_numeric_servname(servname);
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
static const char *msf_unparse_protoname(int proto)
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
        MSF_NETWORK_LOG(DBG_DEBUG, "Unix local[%s] clifd[%d].", cun->sun_path, fd);

    } else if (cliaddr.ss_family == AF_INET) {

        sin = SIN(&cliaddr);

        /* inet_ntoa(sin->sin_addr) */
        inet_ntop(cliaddr.ss_family, &sin->sin_addr, ip, sizeof(ip));
        MSF_NETWORK_LOG(DBG_DEBUG, "Network ipv4 local[%s] port[%d].", ip, ntohs(sin->sin_port));

    } else if (cliaddr.ss_family == AF_INET6) {

        sin6 = SIN6(&cliaddr);
        inet_ntop(cliaddr.ss_family, &sin6->sin6_addr, ip, sizeof(ip));
        MSF_NETWORK_LOG(DBG_DEBUG, "Network ipv6 local[%s] port[%d].", ip, ntohs(sin6->sin6_port));
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

    /* Ôİ²»ÉèÖÃÈ¨ÏŞÊ¹ÓÃÄ¬ÈÏµÄ0777 CLI_PERM */
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
µØÖ·½á¹¹Ìå·ÖÎö
sockaddrºÍsockaddr_inÔÚ×Ö½Ú³¤¶ÈÉÏ¶¼Îª16¸öBYTE,¿ÉÒÔ½øĞĞ×ª»»

struct   sockaddr   {  
    unsigned   short    sa_family;    	 //2 
    char                sa_data[14];     //14
};

struct   sockaddr_in   {  
    short  int              sin_family;   //2
    unsigned   short int    sin_port;     //2
    struct  in_addr         sin_addr;     //4  32Î»IPµØÖ·
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
»òÕß
struct in_addr {
in_addr_t s_addr;
//unsigned long s_addr;
};
½á¹¹Ìåin_addr ÓÃÀ´±íÊ¾Ò»¸ö32Î»µÄIPv4µØÖ·
inet_addr()ÊÇ½«Ò»¸öµã·ÖÖÆµÄIPµØÖ·(Èç192.168.0.1)×ª»»
ÎªÉÏÊö½á¹¹ÖĞĞèÒªµÄ32Î»¶ş½øÖÆ·½Ê½µÄIPµØÖ·(0xC0A80001).
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

//LinuxÏÂ±à³Ì»ñÈ¡±¾µØIPµØÖ·µÄ³£¼û·½·¨

/* ·½·¨Ò»:ioctl()»ñÈ¡±¾µØIPµØÖ· 

Linux ÏÂ ¿ÉÒÔÊ¹ÓÃioctl()º¯ÊıÒÔ¼°½á¹¹Ìå 
struct ifreqºÍ½á¹¹Ìåstruct ifconfÀ´»ñÈ¡ÍøÂç½Ó¿ÚµÄ¸÷ÖÖĞÅÏ¢¡£

¾ßÌå¹ı³ÌÊÇÏÈÍ¨¹ıictol»ñÈ¡±¾µØµÄËùÓĞ½Ó¿ÚĞÅÏ¢,´æ·Åµ½ifconf½á¹¹ÖĞ
ÔÙ´ÓÆäÖĞÈ¡³öÃ¿¸öifreq±íÊ¾µÄipĞÅÏ¢£¨Ò»°ãÃ¿¸öÍø¿¨¶ÔÓ¦Ò»¸öIPµØÖ·,
Èç£º¡±eth0¡­¡¢eth1¡­¡±)¡£

ÏÈÁË½â½á¹¹Ìå struct ifreqºÍ½á¹¹Ìåstruct ifconf:
ifconfÍ¨³£ÊÇÓÃÀ´±£´æËùÓĞ½Ó¿ÚĞÅÏ¢µÄ
Í·ÎÄ¼şif.h
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

//ifreqÓÃÀ´±£´æÄ³¸ö½Ó¿ÚµÄĞÅÏ¢
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
    //Èç¹ûÏë»ñÈ¡ËùÓĞÍøÂç½Ó¿ÚĞÅÏ¢

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

    /* Ô­ÏÈÊ¹ÓÃ snprintf(p_ip, len, "%s", inet_ntoa(addr_temp));
    * ÓÉÓÚinet_ntoaµÄ·µ»ØÖµÊÇ¹²ÓÃÒ»Æ¬ÄÚ´æ£¬¶àÏß³Ì²Ù×÷·µ»ØÖµÓĞ¸ÅÂÊ»á´íÂÒµô
    * Òò´ËÓÃinet_ntopÈ¡´úinet_ntoaº¯Êı
    */
    (void)inet_ntop(AF_INET, &addr_temp, ip, len);

    return 0;
}

s32 get_ipaddr_by_ioaddr(s8* iface, s8* ipaddr, s32 len){
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
/* ·½·¨¶ş£ºgetsockname()»ñÈ¡±¾µØIPµØÖ· 
Èç¹û½¨Á¢TCP,UDPÁ¬½ÓµÄÇé¿öÏÂ
(UDPÎŞ·¨»ñÈ¡¶Ô¶ËµÄĞÅÏ¢,¶øÇÒTCPÇé¿öÏÂ»ñÈ¡µØÖ·ÊôĞÔfamily×¢Òâ,
Òª¿´±¾µØ°ó¶¨µÄĞ­Òé×åÊÇÉ¶,ÓĞĞ©IPV4ºÍIPV6¶¼°ó¶¨µÄÊÇAF_INT6 -hikvision)
¿ÉÒÔÍ¨¹ıgetsocknameºÍgetpeernameº¯ÊıÀ´»ñÈ¡±¾µØºÍ¶Ô¶ËµÄIPºÍ¶Ë¿ÚºÅ

*/

int get_ifaddr_by_sock(char* ip, int slen) {

    int     fd = -1;
    socklen_t len = sizeof(struct sockaddr_in);
    //sockaddr_storage ¡ã¨¹¨¤¡§v6o¨ªv4¦Ì??¡¤
    struct sockaddr_in servaddr;
    struct sockaddr_in localaddr;
    struct sockaddr_in peeraddr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0) {
        return -1;
    }

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family 		= 	AF_INET;
    servaddr.sin_port			=	htons(9000);

    servaddr.sin_addr.s_addr  	= 	htonl(INADDR_ANY); // inet_addr("192.168.0.1");   

    if(connect(fd,(struct sockaddr*)&servaddr,sizeof(servaddr)) < 0 ) {
        return -1;
    }
    char buf[30];
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

/* ·½·¨Èı£ºgetaddrinfo()»ñÈ¡±¾µØIPµØÖ· 
×¢Òâ:getaddrinfo()¿ÉÒÔÍê³ÉÍøÂçÖ÷»úÖĞÖ÷»úÃûºÍ·şÎñÃûµ½µØÖ·µÄÓ³Éä,
µ«ÊÇÒ»°ã²»ÄÜÓÃÀ´»ñÈ¡±¾µØIPµØÖ·,µ±ËüÓÃÀ´»ñÈ¡±¾µØIPµØÖ·Ê±,
·µ»ØµÄÒ»°ãÊÇ127.0.0.1±¾µØ»Ø»·µØÖ·¡£ 


*/

int get_ifaddr_by_addrinfo(char* ip, int len) {

    int  ret = -1;
    char host_name[128] = { 0 };
    gethostname(host_name, sizeof(host_name));
    MSF_NETWORK_LOG(DBG_ERROR, "host_name:%s.", host_name);

    struct addrinfo*	ailist=NULL, *aip=NULL;
    struct sockaddr_in*	saddr;
    char*  addr = NULL;
    ret = getaddrinfo(host_name, NULL,NULL, &ailist);
    for(aip=ailist; aip!=NULL; aip=aip->ai_next) {
        if(aip->ai_family == AF_INET) {
            saddr=(struct sockaddr_in*)aip->ai_addr;
            addr=inet_ntoa(saddr->sin_addr);
        }
        MSF_NETWORK_LOG(DBG_ERROR, "addr:%s.",addr);
    }

    MSF_NETWORK_LOG(DBG_ERROR, "\n-------www.luotang.me host info---------\n");
    getaddrinfo("www.luotang.me", "http", NULL, &ailist);
    for(aip=ailist; aip != NULL; aip=aip->ai_next) {
        if(aip->ai_family == AF_INET) {
            saddr=(struct sockaddr_in*)aip->ai_addr;
            addr=inet_ntoa(saddr->sin_addr);
        }
        MSF_NETWORK_LOG(DBG_ERROR, "www.luotang.me addr:%s.",addr);
    }

    return 0;
}

/*
·½·¨ËÄ£ºgethostname()»ñÈ¡±¾µØIPµØÖ· 
gethostname()ºÍgetaddrinfo()µÄ¹¦ÄÜÀàËÆ,
Ò»°ãÓÃÓÚÍ¨¹ıÖ÷»úÃû»òÕß·şÎñÃû,±ÈÈçÓòÃûÀ´»ñÈ¡Ö÷»úµÄIPµØÖ·.
µ«ÊÇÒªÏë»ñÈ¡±¾µØIPµØÖ·µÄÊ±ºò,Ò»°ã»ñÈ¡µÄÊÇ»Ø»·µØÖ·127.0.0.1

×¢Òâ,Ö÷»úµÄµØÖ·ÊÇÒ»¸öÁĞ±íµÄĞÎÊ½,Ô­ÒòÊÇµ±Ò»¸öÖ÷»úÓĞ¶à¸öÍøÂç½Ó¿ÚÊ±,
¼°¶à¿éÍø¿¨»òÕßÒ»¸öÍø¿¨°ó¶¨¶à¸öIPµØÖ·Ê±,×ÔÈ»¾ÍÓĞ¶à¸öIPµØÖ·.
ÒÔÉÏ´úÂë»ñÈ¡µÄÊÇ¸ù¾İÖ÷»úÃû³ÆµÃµ½µÄµÚÒ»¸öIPµØÖ·.
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
·½·¨Îå£ºgetifaddrs()»ñÈ¡±¾µØIPµØÖ·
²éÕÒµ½ÏµÍ³ËùÓĞµÄÍøÂç½Ó¿ÚµÄĞÅÏ¢,°üÀ¨ÒÔÌ«Íø¿¨½Ó¿ÚºÍ»Ø»·½Ó¿ÚµÈ
glibcµÄÔ´Âë,ËüÊÇÀûÓÃnetlinkÀ´ÊµÏÖµÄ,
ËùÒÔÔÚÊ¹ÓÃÕâ¸ö½Ó¿ÚµÄÊ±ºòÒ»¶¨ÒªÈ·±£ÄãµÄÄÚºËÖ§³Önetlink.
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
½«EPOLLÉèÎª±ßÔµ´¥·¢(Edge Triggered)Ä£Ê½,
ÕâÊÇÏà¶ÔÓÚË®Æ½´¥·¢(Level Triggered)À´ËµµÄ

EPOLLÊÂ¼şÓĞÁ½ÖÖÄ£ĞÍ£º
Edge Triggered(ET)	 
¸ßËÙ¹¤×÷·½Ê½,´íÎóÂÊ±È½Ï´ó,Ö»Ö§³Öno_block socket (·Ç×èÈûsocket)
LevelTriggered(LT)	 
È±Ê¡¹¤×÷·½Ê½£¬¼´Ä¬ÈÏµÄ¹¤×÷·½Ê½,Ö§³ÖblocksocketºÍno_blocksocket,´íÎóÂÊ±È½ÏĞ¡.

EPOLLIN:
listen fd,ÓĞĞÂÁ¬½ÓÇëÇó,¶Ô¶Ë·¢ËÍÆÕÍ¨Êı¾İ

EPOLLPRI:
ÓĞ½ô¼±µÄÊı¾İ¿É¶Á(ÕâÀïÓ¦¸Ã±íÊ¾ÓĞ´øÍâÊı¾İµ½À´)

EPOLLERR:
±íÊ¾¶ÔÓ¦µÄÎÄ¼şÃèÊö·û·¢Éú´íÎó

EPOLLHUP:
±íÊ¾¶ÔÓ¦µÄÎÄ¼şÃèÊö·û±»¹Ò¶Ï 
¶Ô¶ËÕı³£¹Ø±Õ(³ÌĞòÀïclose(),shellÏÂkill»òctr+c),´¥·¢EPOLLINºÍEPOLLRDHUP,
µ«ÊÇ²»´¥·¢EPOLLERR ºÍEPOLLHUP.ÔÙman epoll_ctl¿´ÏÂºóÁ½¸öÊÂ¼şµÄËµÃ÷,
ÕâÁ½¸öÓ¦¸ÃÊÇ±¾¶Ë£¨server¶Ë£©³ö´í²Å´¥·¢µÄ.

EPOLLRDHUP:
Õâ¸öºÃÏñÓĞĞ©ÏµÍ³¼ì²â²»µ½,¿ÉÒÔÊ¹ÓÃEPOLLIN,read·µ»Ø0,É¾³ıµôÊÂ¼ş,¹Ø±Õclose(fd)

EPOLLONESHOT:
Ö»¼àÌıÒ»´ÎÊÂ¼ş,µ±¼àÌıÍêÕâ´ÎÊÂ¼şÖ®ºó,
Èç¹û»¹ĞèÒª¼ÌĞø¼àÌıÕâ¸ösocketµÄ»°,
ĞèÒªÔÙ´Î°ÑÕâ¸ösocket¼ÓÈëµ½EPOLL¶ÓÁĞÀï

¶Ô¶ËÒì³£¶Ï¿ªÁ¬½Ó(Ö»²âÁË°ÎÍøÏß),Ã»´¥·¢ÈÎºÎÊÂ¼ş¡£

epollµÄÓÅµã£º
1.Ö§³ÖÒ»¸ö½ø³Ì´ò¿ª´óÊıÄ¿µÄsocketÃèÊö·û(FD)
select ×î²»ÄÜÈÌÊÜµÄÊÇÒ»¸ö½ø³ÌËù´ò¿ªµÄFDÊÇÓĞÒ»¶¨ÏŞÖÆµÄ£¬
ÓÉFD_SETSIZEÉèÖÃ£¬Ä¬ÈÏÖµÊÇ2048¡£
¶ÔÓÚÄÇĞ©ĞèÒªÖ§³ÖµÄÉÏÍòÁ¬½ÓÊıÄ¿µÄIM·şÎñÆ÷À´ËµÏÔÈ»Ì«ÉÙÁË¡£
ÕâÊ±ºòÄãÒ»ÊÇ¿ÉÒÔÑ¡ÔñĞŞ¸ÄÕâ¸öºêÈ»ºóÖØĞÂ±àÒëÄÚºË£¬
²»¹ı×ÊÁÏÒ²Í¬Ê±Ö¸³öÕâÑù»á´øÀ´ÍøÂçĞ§ÂÊµÄÏÂ½µ£¬
¶şÊÇ¿ÉÒÔÑ¡Ôñ¶à½ø³ÌµÄ½â¾ö·½°¸(´«Í³µÄ Apache·½°¸)£¬
²»¹ıËäÈ»linuxÉÏÃæ´´½¨½ø³ÌµÄ´ú¼Û±È½ÏĞ¡£¬µ«ÈÔ¾ÉÊÇ²»¿ÉºöÊÓµÄ£¬
¼ÓÉÏ½ø³Ì¼äÊı¾İÍ¬²½Ô¶±È²»ÉÏÏß³Ì¼äÍ¬²½µÄ¸ßĞ§£¬ËùÒÔÒ²²»ÊÇÒ»ÖÖÍêÃÀµÄ·½°¸¡£
²»¹ı epollÔòÃ»ÓĞÕâ¸öÏŞÖÆ£¬ËüËùÖ§³ÖµÄFDÉÏÏŞÊÇ×î´ó¿ÉÒÔ´ò¿ªÎÄ¼şµÄÊıÄ¿£¬
Õâ¸öÊı×ÖÒ»°ãÔ¶´óÓÚ2048,¾Ù¸öÀı×Ó,ÔÚ1GBÄÚ´æµÄ»úÆ÷ÉÏ´óÔ¼ÊÇ10Íò×óÓÒ£¬
¾ßÌåÊıÄ¿¿ÉÒÔcat /proc/sys/fs/file-max²ì¿´,Ò»°ãÀ´ËµÕâ¸öÊıÄ¿ºÍÏµÍ³ÄÚ´æ¹ØÏµºÜ´ó¡£

2.IOĞ§ÂÊ²»ËæFDÊıÄ¿Ôö¼Ó¶øÏßĞÔÏÂ½µ
´«Í³µÄselect/pollÁíÒ»¸öÖÂÃüÈõµã¾ÍÊÇµ±ÄãÓµÓĞÒ»¸öºÜ´óµÄsocket¼¯ºÏ£¬
²»¹ıÓÉÓÚÍøÂçÑÓÊ±£¬ÈÎÒ»Ê±¼äÖ»ÓĞ²¿·ÖµÄsocketÊÇ"»îÔ¾"µÄ£¬
µ«ÊÇselect/pollÃ¿´Îµ÷ÓÃ¶¼»áÏßĞÔÉ¨ÃèÈ«²¿µÄ¼¯ºÏ£¬µ¼ÖÂĞ§ÂÊ³ÊÏÖÏßĞÔÏÂ½µ¡£
µ«ÊÇepoll²»´æÔÚÕâ¸öÎÊÌâ£¬ËüÖ»»á¶Ô"»îÔ¾"µÄsocket½øĞĞ²Ù×÷---
ÕâÊÇÒòÎªÔÚÄÚºËÊµÏÖÖĞepollÊÇ¸ù¾İÃ¿¸öfdÉÏÃæµÄcallbackº¯ÊıÊµÏÖµÄ¡£
ÄÇÃ´£¬Ö»ÓĞ"»îÔ¾"µÄsocket²Å»áÖ÷¶¯µÄÈ¥µ÷ÓÃ callbackº¯Êı£¬
ÆäËûidle×´Ì¬socketÔò²»»á£¬ÔÚÕâµãÉÏ£¬epollÊµÏÖÁËÒ»¸ö"Î±"AIO£¬
ÒòÎªÕâÊ±ºòÍÆ¶¯Á¦ÔÚosÄÚºË¡£ÔÚÒ»Ğ© benchmarkÖĞ£¬
Èç¹ûËùÓĞµÄsocket»ù±¾ÉÏ¶¼ÊÇ»îÔ¾µÄ---±ÈÈçÒ»¸ö¸ßËÙLAN»·¾³£¬
epoll²¢²»±Èselect/pollÓĞÊ²Ã´Ğ§ÂÊ£¬Ïà·´£¬Èç¹û¹ı¶àÊ¹ÓÃepoll_ctl,
Ğ§ÂÊÏà±È»¹ÓĞÉÔÎ¢µÄÏÂ½µ¡£µ«ÊÇÒ»µ©Ê¹ÓÃidle connectionsÄ£ÄâWAN»·¾³,
epollµÄĞ§ÂÊ¾ÍÔ¶ÔÚselect/pollÖ®ÉÏÁË

3.Ê¹ÓÃmmap¼ÓËÙÄÚºËÓëÓÃ»§¿Õ¼äµÄÏûÏ¢´«µİ
ÕâµãÊµ¼ÊÉÏÉæ¼°µ½epollµÄ¾ßÌåÊµÏÖÁË¡£
ÎŞÂÛÊÇselect,poll»¹ÊÇepoll¶¼ĞèÒªÄÚºË°ÑFDÏûÏ¢Í¨Öª¸øÓÃ»§¿Õ¼ä£¬
ÈçºÎ±ÜÃâ²»±ØÒªµÄÄÚ´æ¿½±´¾ÍºÜÖØÒª£¬ÔÚÕâµãÉÏ£¬
epollÊÇÍ¨¹ıÄÚºËÓÚÓÃ»§¿Õ¼ämmapÍ¬Ò»¿éÄÚ´æÊµÏÖµÄ¡£
¶øÈç¹ûÄãÏëÎÒÒ»Ñù´Ó2.5ÄÚºË¾Í¹Ø×¢epollµÄ»°£¬Ò»¶¨²»»áÍü¼ÇÊÖ¹¤ mmapÕâÒ»²½µÄ¡£

4.ÄÚºËÎ¢µ÷
ÕâÒ»µãÆäÊµ²»ËãepollµÄÓÅµãÁË£¬¶øÊÇÕû¸ölinuxÆ½Ì¨µÄÓÅµã¡£
Ò²ĞíÄã¿ÉÒÔ»³ÒÉlinuxÆ½Ì¨£¬µ«ÊÇÄãÎŞ·¨»Ø±ÜlinuxÆ½Ì¨¸³ÓèÄãÎ¢µ÷ÄÚºËµÄÄÜÁ¦¡£
±ÈÈç£¬ÄÚºËTCP/IPĞ­ÒéÕ»Ê¹ÓÃÄÚ´æ³Ø¹ÜÀísk_buff½á¹¹£¬
ÄÇÃ´¿ÉÒÔÔÚÔËĞĞÊ±ÆÚ¶¯Ì¬µ÷ÕûÕâ¸öÄÚ´æpool(skb_head_pool)µÄ´óĞ¡--- 
Í¨¹ıechoXXXX>/proc/sys/net/core/hot_list_lengthÍê³É¡£
ÔÙ±ÈÈçlistenº¯ÊıµÄµÚ2¸ö²ÎÊı(TCPÍê³É3´ÎÎÕÊÖµÄÊı¾İ°ü¶ÓÁĞ³¤¶È)£¬
Ò²¿ÉÒÔ¸ù¾İÄãÆ½Ì¨ÄÚ´æ´óĞ¡¶¯Ì¬µ÷Õû¡£
¸üÉõÖÁÔÚÒ»¸öÊı¾İ°üÃæÊıÄ¿¾Ş´óµ«Í¬Ê±Ã¿¸öÊı¾İ°ü±¾Éí´óĞ¡È´ºÜĞ¡µÄÌØÊâÏµÍ³ÉÏ
³¢ÊÔ×îĞÂµÄNAPIÍø¿¨Çı¶¯¼Ü¹¹¡£


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
                /* ?-?¨¨¨º1¨®? snprintf(p_gateway, len, "%s", inet_ntoa(gw));;
                * ¨®¨¦¨®¨²inet_ntoa¦Ì?¡¤¦Ì???¦Ì¨º?12¨®?¨°????¨²¡ä?¡ê??¨¤??3¨¬2¨´¡Á¡Â¡¤¦Ì???¦Ì¨®D???¨º?¨¢¡ä¨ª?¨°¦Ì?
                * ¨°¨°¡ä?¨®?inet_ntop¨¨?¡ä¨²inet_ntoao¡¥¨ºy
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
    /* ?-?¨¨¨º1¨®? snprintf(p_netmask, len, "%s", inet_ntoa(addr_temp));
    * ¨®¨¦¨®¨²inet_ntoa¦Ì?¡¤¦Ì???¦Ì¨º?12¨®?¨°????¨²¡ä?¡ê??¨¤??3¨¬2¨´¡Á¡Â¡¤¦Ì???¦Ì¨®D???¨º?¨¢¡ä¨ª?¨°¦Ì?
    * ¨°¨°¡ä?¨®?inet_ntop¨¨?¡ä¨²inet_ntoao¡¥¨ºy
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

    /* ?-?¨¨¨º1¨®? snprintf(p_broadcast, len, "%s", inet_ntoa(addr_temp));
    * ¨®¨¦¨®¨²inet_ntoa¦Ì?¡¤¦Ì???¦Ì¨º?12¨®?¨°????¨²¡ä?¡ê??¨¤??3¨¬2¨´¡Á¡Â¡¤¦Ì???¦Ì¨®D???¨º?¨¢¡ä¨ª?¨°¦Ì?
    * ¨°¨°¡ä?¨®?inet_ntop¨¨?¡ä¨²inet_ntoao¡¥¨ºy
    */
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

    /*?¡ã¡Áo3¡è?¨¨o?¡¤¡§?¦Ì?a3~127*/
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

/**   
*  @brief  ¨¦?3y???¡§IPv6¦Ì??¡¤???¡ã¨°??-¨¦¨¨?¡§¦Ì?¨ª?1?	   
*  @param[in]  ipaddr IPv6¦Ì??¡¤¡ê?¡¤??¡ì:¡¤?NULL
*  @param[out] ?T
*  @return 0 3¨¦1|; -1 ¨º¡ì¡ã¨¹  
*/
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
    /*¨¦?3y?¡¤¨®¨¦*/
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

    /*DT???a¨ª?1?¦Ì???¡À¨º¦Ì??¡¤?a¨¨?¨¢?¦Ì??¡¤,¡¤??¨°?¨¢¦Ì???¨ºy?Y???¨¹¡ä??????¡§¦Ì?¨°???¡Á¨®¨ª?*/
    memcpy(&valid_ipv6, &g_any6addr, sizeof(valid_ipv6));
    /*¨¦?3y??IPv6¦Ì??¡¤¦Ì??¨´¨®D¨ª?1?*/
    while(del_v6_gateway(&valid_ipv6) == 0);

    if ((sockfd = socket(PF_INET6, SOCK_DGRAM, 0)) < 0){		 
        return -1;  
    }

    memset(&v6_rt, 0, sizeof(struct in6_rtmsg));

    memcpy(&v6_rt.rtmsg_dst, &valid_ipv6, sizeof(struct in6_addr));
    v6_rt.rtmsg_flags = RTF_UP | RTF_GATEWAY;

    /*??¡À¨º¦Ì??¡¤?¨²??3¡è?¨¨?a0¡ê??¡ä??¡À¨º¦Ì??¡¤?a?¨´¨®DIP*/
    v6_rt.rtmsg_dst_len = 0;
    v6_rt.rtmsg_metric = 1;

    memset(&ifr, 0, sizeof(ifr));
    /*¨ª???¨¦¨¨¡À???:'eth0'*/
    memcpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name));
    (void)ioctl(sockfd, SIOGIFINDEX, &ifr);
    v6_rt.rtmsg_ifindex = ifr.ifr_ifindex;

    memcpy(&v6_rt.rtmsg_gateway, gateway, sizeof(struct in6_addr));
    /*¨¬¨ª?¨®?¡¤¨®¨¦*/
    if (ioctl(sockfd, SIOCADDRT, &v6_rt) < 0){
        sclose(sockfd);
        MSF_NETWORK_LOG(DBG_ERROR, "add route ioctl error and errno=%d.", errno);
        return -1;
    }
    sclose(sockfd);

    return 0;
}
 

/**   
*  @brief  ¡ä¨®if_inet6???t?D??¨¨?¨¦¨¨¡À?¨®DD¡ì¦Ì?IPv6¦Ì??¡¤	
*  @param[in]  device_name ¨ª???¨¦¨¨¡À????¡ê¨¤y¨¨?"eth0"
*  @param[in]  ipv6 IPv6¦Ì??¡¤?¡ê¡¤??¡ì:¡¤?NULL
*  @param[in]  prefix ¡Á¨®¨ª??¨²??3¡è?¨¨
*  @param[out] ipv6 ??¨¨?¦Ì?¦Ì?¨®DD¡ì¦Ì?ipv6¦Ì??¡¤
*  @param[out] prefix ¨®DD¡ìipv6¦Ì??¡¤¦Ì?¡Á¨®¨ª??¨²??3¡è?¨¨
*  @return 0 3¨¦1|; -1 ¨º¡ì¡ã¨¹  
*/
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
    inv_addr2.s6_addr[14] = 0x10;/*0:0:0:0:0:0:0:1 ?¡¤??¦Ì??¡¤*/

#if 0
    if(get_ipv6addr_num(device_name, &ipv61, &prefix1, &ipv62, &prefix2) != OK)
    {
    MSF_NETWORK_LOG(DBG_ERROR, "update_net_param-- get_ipv6addr_num failed,errno:%d.",errno);  
    }
    else
    {
    if(!IN6_IS_ADDR_UNSPECIFIED(&ipv61))		 
    {
     if (IN6_IS_ADDR_LINKLOCAL(&ipv61) || IN6_IS_ADDR_LOOPBACK(&ipv61))  /*IPv61?TD¡ì*/  
     {
        if (!IN6_IS_ADDR_UNSPECIFIED(&ipv62) && !IN6_IS_ADDR_LINKLOCAL(&ipv62) && !IN6_IS_ADDR_LOOPBACK(&ipv62))
        {
            // IPv62¨º?¨®DD¡ì¦Ì??¡¤
            memcpy(ipv6, &ipv62, sizeof(ipv62));
            *prefix = prefix2;
             
            return 0;
        }

        /*IPv61?¡éIPv62???TD¡ì*/		 
        memcpy(ipv6, &g_any6addr, sizeof(g_any6addr));

        return 0;
    {
    else
    {
         // IPv61¨º?¨®DD¡ì¦Ì??¡¤
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
 
int get_dns(char* p_dns1, char* p_dns2)
{
    char    buf[128], name[50];
    char*   dns[2];
    int     dns_index = 0;
    FILE*   fp = NULL;

    memset(buf, 0, sizeof(buf));
    memset(name, 0, sizeof(name));
    memset(dns, 0, sizeof(dns));

    if (! p_dns1 || !p_dns2){
        MSF_NETWORK_LOG(DBG_ERROR, "get_dns: param err.");
        return -1;
    }

    fp = fopen(RESOLV_FILE, "r");
    if(NULL == fp){
        MSF_NETWORK_LOG(DBG_ERROR, "can not open file /etc/resolv.conf.");
        return -1;
    }

    dns[0] = p_dns1;
    dns[1] = p_dns2;

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

    // ??¨¨?¨ª???¨¦¨¨¡À??¨®?¨²¦Ì?????D??¡é?¨´D¨¨¦Ì??¨²¡ä?
    (void)ioctl(fd, SIOCGIFADDR, &ifreq);
    memcpy(&mreq.imr_interface, &((struct sockaddr_in*)&ifreq.ifr_addr)->sin_addr,
     sizeof(struct in_addr));

    // ??IP?¨®¨¨??¨¤2£¤¡Á¨¦
    return setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
}
 

/**@brief	 ?¨®¨¨??¨¤2£¤¡Á¨¦,?¨¹1???¨¨YIPv6,	   
* @param[in]	socket_fd ¡ä¡ä?¡§¦Ì?¨¢??¨®??¡À¨²
* @param[in]	p_mcastgroupaddr ?¨¤2£¤¡Á¨¦¦Ì?¦Ì??¡¤
* @param[out] ?T
* @return    OK/ERROR
*/
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
        // ??¨¨?¨ª???¨¦¨¨¡À??¨®?¨²¦Ì?????D??¡é?¨´D¨¨¦Ì??¨²¡ä?
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

/** @fn int mcast_leave(INT32 socket_fd, struct sockaddr* p_addrs, INT32 len)	   
*  @brief  ?????¡§IP¡ä¨®?¨¤2£¤¡Á¨¦?D¨¦?3y?¡ê¨¨?¡¤¦Ì??OK,¡À¨ª¨º?¨¦?3y3¨¦1|?¡ê¨¨?¡¤¦Ì??ERROR,¡À¨ª¨º?¨¦?3y¨º¡ì¡ã¨¹?¡ê
*  @param[in]  socket_fd socket??¡À¨²?¡ê¡¤??¡ì:¡ä¨®¨®¨²0
*  @param[in]  addr ¡ã¨¹o?¡¤t???¡Âipo¨ª???¨²o??¡ê¡¤??¡ì:¡¤?NULL
*  @param[in]  len addr¦Ì?3¡è?¨¨?¡ê¡¤??¡ì:¡ä¨®¨®¨²0
*  @param[out] ?T
*  @return   OK/ERROR
*/
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
 
/**@brief	 ?????¡§IP¡ä¨®?¨¤2£¤¡Á¨¦?D¨¦?3y,??¨¨YIPv6.¨¨?¡¤¦Ì??OK,¡À¨ª¨º?¨¦?3y3¨¦1|?¡ê¨¨?¡¤¦Ì??ERROR,¡À¨ª¨º?¨¦?3y¨º¡ì¡ã¨¹?¡ê   
* @param[in]	socket_fd ¡ä¡ä?¡§¦Ì?¨¢??¨®??¡À¨²
* @param[in]	p_mcastgroupaddr ?¨¤2£¤¡Á¨¦¦Ì?¦Ì??¡¤
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
        // ??¨¨?¨ª???¨¦¨¨¡À??¨®?¨²¦Ì?????D??¡é?¨´D¨¨¦Ì??¨²¡ä?
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
 
 
/** @fn INT32 mcast_setloop(INT32 socket, INT32 enable)   
*  @brief  ¨¦¨¨???¨¤2£¤¡Á¨¦?¨¦¨°?¨®D???¡¤?¡ê¨¨?¡¤¦Ì??OK,¡À¨ª¨º?¨¦¨¨??3¨¦1|?¡ê¨¨?¡¤¦Ì??ERROR,¡À¨ª¨º?¨¦¨¨??¨º¡ì¡ã¨¹  
*  @param[in]  socket_fd ¨ª???¨¬¡Á?¨®¡Á?
*  @param[in]  enable ???¡§¨º?¡¤?¨®D???¡¤?¡ê¡¤??¡ì:0,1?¡ê0¡À¨ª¨º???¨®D???¡¤;1¡À¨ª¨º??¨¦¨°?¨®D???¡¤
*  @param[out] ?T
*  @return   OK/ERROR
*/
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
 
 
/**@brief	 ¨¦¨¨???¨¤2£¤¡Á¨¦?¨¦¨°?¨®D???¡¤,?¡ì3?IPv6?¡ê¨¨?¡¤¦Ì??OK,¡À¨ª¨º?¨¦¨¨??3¨¦1|?¡ê¨¨?¡¤¦Ì??ERROR,¡À¨ª¨º?¨¦¨¨??¨º¡ì¡ã¨¹	   
* @param[in]	
* @param[out] 
* @return    
*/
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
    if (get_ipaddr_by_ioaddr(if_name, tmp, sizeof(tmp)) != 0 ){
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

    // ¡¤¨¤?1¨ª?1?¡À?¨¦¨¨???a0.0.0.0
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
* NetlinkÌ×½Ó×ÖÊÇÓÃÒÔÊµÏÖÓÃ»§½ø³ÌÓëÄÚºË½ø³ÌÍ¨ĞÅµÄÒ»ÖÖÌØÊâµÄ½ø³Ì¼äÍ¨ĞÅ(IPC),
* Ò²ÊÇÍøÂçÓ¦ÓÃ³ÌĞòÓëÄÚºËÍ¨ĞÅµÄ×î³£ÓÃµÄ½Ó¿Ú.
* Ê¹ÓÃnetlink ½øĞĞÓ¦ÓÃÓëÄÚºËÍ¨ĞÅµÄÓ¦ÓÃºÜ¶à:
* °üÀ¨: 
    1. NETLINK_ROUTE:ÓÃ»§¿Õ¼äÂ·ÓÉdamon,ÈçBGP,OSPF,RIPºÍÄÚºË°ü×ª·¢Ä£¿éµÄÍ¨ĞÅĞÅµÀ
    ÓÃ»§¿Õ¼äÂ·ÓÉdamonÍ¨¹ı´ËÖÖnetlinkĞ­ÒéÀàĞÍ¸üĞÂÄÚºËÂ·ÓÉ±í
    2. NETLINK_FIREWALL:½ÓÊÕIPv4·À»ğÇ½´úÂë·¢ËÍµÄ°ü
    3. NETLINK_NFLOG:ÓÃ»§¿Õ¼äiptable¹ÜÀí¹¤¾ßºÍÄÚºË¿Õ¼äNetfilterÄ£¿éµÄÍ¨ĞÅĞÅµÀ
    4. NETLINK_ARPD:ÓÃ»§¿Õ¼ä¹ÜÀíarp±í
    5. NETLINK_USERSOCK:ÓÃ»§Ì¬socketĞ­Òé
    6. NETLINK_NETFILTER:netfilter×ÓÏµÍ³
    7. NETLINK_KOBJECT_UEVENT:ÄÚºËÊÂ¼şÏòÓÃ»§Ì¬Í¨Öª
    8. NETLINK_GENERIC:Í¨ÓÃnetlink
*
*  Netlink ÊÇÒ»ÖÖÔÚÄÚºËÓëÓÃ»§Ó¦ÓÃ¼ä½øĞĞË«ÏòÊı¾İ´«ÊäµÄ·Ç³£ºÃµÄ·½Ê½,
*  ÓÃ»§Ì¬Ó¦ÓÃÊ¹ÓÃ±ê×¼µÄ socket API ¾Í¿ÉÒÔÊ¹ÓÃ netlink Ìá¹©µÄÇ¿´ó¹¦ÄÜ,
*  ÄÚºËÌ¬ĞèÒªÊ¹ÓÃ×¨ÃÅµÄÄÚºË API À´Ê¹ÓÃ netlink
*  1. netlinkÊÇÒ»ÖÖÒì²½Í¨ĞÅ»úÖÆ, ÔÚÄÚºËÓëÓÃ»§Ì¬Ó¦ÓÃÖ®¼ä´«µİµÄÏûÏ¢±£´æÔÚsocket
*  »º´æ¶ÓÁĞÖĞ£¬·¢ËÍÏûÏ¢Ö»ÊÇ°ÑÏûÏ¢±£´æÔÚ½ÓÊÕÕßµÄsocketµÄ½ÓÊÕ¶ÓÁĞ,
*  ¶ø²»ĞèÒªµÈ´ı½ÓÊÕÕßÊÕµ½ÏûÏ¢, ËüÌá¹©ÁËÒ»¸ösocket¶ÓÁĞÀ´Æ½»¬Í»·¢µÄĞÅÏ¢
*  2. Ê¹ÓÃ netlink µÄÄÚºË²¿·Ö¿ÉÒÔ²ÉÓÃÄ£¿éµÄ·½Ê½ÊµÏÖ,Ê¹ÓÃ netlink µÄÓ¦ÓÃ²¿·ÖºÍÄÚºË²¿·ÖÃ»ÓĞ±àÒëÊ±ÒÀÀµ
*  3. netlink Ö§³Ö¶à²¥, ÄÚºËÄ£¿é»òÓ¦ÓÃ¿ÉÒÔ°ÑÏûÏ¢¶à²¥¸øÒ»¸önetlink×é,
*   ÊôÓÚ¸Ãneilink ×éµÄÈÎºÎÄÚºËÄ£¿é»òÓ¦ÓÃ¶¼ÄÜ½ÓÊÕµ½¸ÃÏûÏ¢,
*   ÄÚºËÊÂ¼şÏòÓÃ»§Ì¬µÄÍ¨Öª»úÖÆ¾ÍÊ¹ÓÃÁËÕâÒ»ÌØĞÔ
*  4. ÄÚºË¿ÉÒÔÊ¹ÓÃ netlink Ê×ÏÈ·¢Æğ»á»°
 5. netlink²ÉÓÃ×Ô¼º¶ÀÁ¢µÄµØÖ·±àÂë, struct sockaddr_nl£»
 6.Ã¿¸öÍ¨¹ınetlink·¢³öµÄÏûÏ¢¶¼±ØĞë¸½´øÒ»¸önetlink×Ô¼ºµÄÏûÏ¢Í·,struct nlmsghdr

*
*  7. ÓÃ»§Ì¬Ó¦ÓÃÊ¹ÓÃ±ê×¼µÄ socket APIÓĞsendto(), recvfrom(),sendmsg(), recvmsg()

 ÔÚ»ùÓÚnetlinkµÄÍ¨ĞÅÖĞ,ÓĞÁ½ÖÖ¿ÉÄÜµÄÇéĞÎ»áµ¼ÖÂÏûÏ¢¶ªÊ§:
 1¡¢ÄÚ´æºÄ¾¡,Ã»ÓĞ×ã¹»¶àµÄÄÚ´æ·ÖÅä¸øÏûÏ¢
 2¡¢»º´æ¸´Ğ´,½ÓÊÕ¶ÓÁĞÖĞÃ»ÓĞ¿Õ¼ä´æ´¢ÏûÏ¢,ÕâÔÚÄÚºË¿Õ¼äºÍÓÃ»§¿Õ¼äÖ®¼äÍ¨ĞÅ
	Ê±¿ÉÄÜ»á·¢Éú»º´æ¸´Ğ´ÔÚÒÔÏÂÇé¿öºÜ¿ÉÄÜ»á·¢Éú:
 3¡¢ÄÚºË×ÓÏµÍ³ÒÔÒ»¸öºã¶¨µÄËÙ¶È·¢ËÍnetlinkÏûÏ¢,µ«ÊÇÓÃ»§Ì¬¼àÌıÕß´¦Àí¹ıÂı
 4¡¢ÓÃ»§´æ´¢ÏûÏ¢µÄ¿Õ¼ä¹ıĞ¡
 Èç¹ûnetlink´«ËÍÏûÏ¢Ê§°Ü,ÄÇÃ´recvmsg()º¯Êı»á·µ»ØNo buffer spaceavailable(ENOBUFS)´íÎó
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

    /* µÚÒ»¸ö²ÎÊı±ØĞëÊÇPF_NETLINK»òÕßAF_NETLINK,
    µÚ¶ş¸ö²ÎÊıÓÃSOCK_DGRAMºÍSOCK_RAW¶¼Ã»ÎÊÌâ,
    µÚÈı¸ö²ÎÊı¾ÍÊÇnetlinkµÄĞ­ÒéºÅ*/

    nl_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_TEST);
    if (nl_fd < 0) return -1;

    memset(&saddr, 0, sizeof(saddr));
    saddr.nl_family = AF_NETLINK;
    saddr.nl_pid = getpid();  
    saddr.nl_groups = 0;

    /*
    nl_pid¾ÍÊÇÒ»¸öÔ¼¶¨µÄÍ¨ĞÅ¶Ë¿Ú£¬ÓÃ»§Ì¬Ê¹ÓÃµÄÊ±ºòĞèÒªÓÃÒ»¸ö·Ç0µÄÊı×Ö£¬
    Ò»°ãÀ´ Ëµ¿ÉÒÔÖ±½Ó²ÉÓÃÉÏ²ãÓ¦ÓÃµÄ½ø³ÌID£¨²»ÓÃ½ø³ÌIDºÅÂëÒ²Ã»ÊÂ£¬
    Ö»ÒªÏµÍ³ÖĞ²»³åÍ»µÄÒ»¸öÊı×Ö¼´¿ÉÊ¹ÓÃ£©¡£
    ¶ÔÓÚÄÚºËµÄµØÖ·£¬¸ÃÖµ±ØĞëÓÃ0£¬Ò²¾ÍÊÇËµ£¬
    Èç¹ûÉÏ²ã Í¨¹ısendtoÏòÄÚºË·¢ËÍnetlinkÏûÏ¢£¬peer addrÖĞnl_pid±ØĞëÌîĞ´0¡

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
    daddr.nl_pad = 0;			/*always set to zero*/	
    daddr.nl_pid = 0;			/*kernel's pid is zero*/  
    daddr.nl_groups = 0;		/*multicast groups mask, if unicast set to zero*/  


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
