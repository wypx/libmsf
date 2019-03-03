
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

#define MSF_LISTEN_BACKLOG  64


//打开频道，使用频道这种方式通信前必须发送的命令
#define MSF_CMD_OPEN_CHANNEL   1
//关闭已经打开的频道，实际上也就是关闭套接字
#define MSF_CMD_CLOSE_CHANNEL  2
//要求接收方正常地退出进程
#define MSF_CMD_QUIT           3
//要求接收方强制地结束进程
#define MSF_CMD_TERMINATE      4
//要求接收方重新打开进程已经打开过的文件
#define MSF_CMD_REOPEN         5


/* 封装了父子进程之间传递的信息
 * 父进程创建好子进程后,通过这个把该子进程的
 * 相关信息全部传递给其他所有进程,这样其他子进程就知道该进程的channel信息了.
 * 因为Nginx仅用这个频道同步master进程与worker进程间的状态 */

struct msf_channel {	
	 u32  	cmd;
     pid_t  pid;
     s32    slot; 	/*在全局进程表中的位置*/
     s32    fd; 	/* 传递进程文件描述符 */
} __attribute__((__packed__));


s32 msf_write_channel(s32 fd, struct msf_channel *ch, size_t size);
s32 msf_read_channel(s32 fd, struct msf_channel *ch, size_t size);
s32 msf_add_channel_event(s32 fd, s32 event);
void msf_close_channel(s32 *fd);


