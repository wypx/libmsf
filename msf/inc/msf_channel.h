
#include <msf_utils.h>

#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/uio.h>

#include <limits.h>             /* IOV_MAX */
#include <sys/sendfile.h>
#include <sys/eventfd.h>
#include <linux/aio_abi.h>

#define MSF_CMD_OPEN_CHANNEL   1
#define MSF_CMD_CLOSE_CHANNEL  2
#define MSF_CMD_QUIT           3
#define MSF_CMD_TERMINATE      4
#define MSF_CMD_REOPEN         5

#if (__FreeBSD__) && (__FreeBSD_version < 400017)
#include <sys/param.h>          /* ALIGN() */
/*
 * FreeBSD 3.x has no CMSG_SPACE() and CMSG_LEN() 
 * and has the broken CMSG_DATA()
 */
#undef  CMSG_SPACE
#define CMSG_SPACE(l)       (ALIGN(sizeof(struct cmsghdr)) + ALIGN(l))

#undef  CMSG_LEN
#define CMSG_LEN(l)         (ALIGN(sizeof(struct cmsghdr)) + (l))

#undef  CMSG_DATA
#define CMSG_DATA(cmsg)     ((u8*)(cmsg) + ALIGN(sizeof(struct cmsghdr)))
#endif

struct msf_channel {
     u32    cmd;
     pid_t  pid;
     s32    slot;/*in global processes*/
     s32    fd;
} MSF_PACKED_MEMORY;

s32 msf_write_channel(s32 fd, struct msf_channel *ch, size_t size);
s32 msf_read_channel(s32 fd, struct msf_channel *ch, size_t size);
s32 msf_add_channel_event(s32 fd, s32 event);
void msf_close_channel(s32 *fd);

