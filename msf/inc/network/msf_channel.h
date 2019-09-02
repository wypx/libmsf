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
#ifndef __MSF_CHANNEL_H__
#define __MSF_CHANNEL_H__

#include <msf_errno.h>
#include <msf_network.h>
#include <msf_log.h>

#define MSF_CMD_OPEN_CHANNEL   1
#define MSF_CMD_CLOSE_CHANNEL  2
#define MSF_CMD_QUIT           3
#define MSF_CMD_TERMINATE      4
#define MSF_CMD_REOPEN         5


#if (__FreeBSD__) && (__FreeBSD_version < 400017)
#include <sys/param.h>          /* ALIGN() */

struct cmsghdr {
    socklen_t cmsg_len;
    s32       cmsg_level;
    s32       cmsg_type;
    /* u_char     cmsg_data[]; */
};

/*
 * FreeBSD 3.x has no CMSG_SPACE() and CMSG_LEN() 
 * and has the broken CMSG_DATA()
 */
#undef  CMSG_SPACE
#define CMSG_SPACE(l)       (MSF_ALIGN(sizeof(struct cmsghdr)) + MSF_ALIGN(l))

#undef  CMSG_LEN
#define CMSG_LEN(l)         (MSF_ALIGN(sizeof(struct cmsghdr)) + (l))

#undef  CMSG_DATA
#define CMSG_DATA(cmsg)     ((u8*)(cmsg) + MSF_ALIGN(sizeof(struct cmsghdr)))
#endif

struct msf_channel_hdr {
    u32     cmd;
    pid_t   pid;
    u32     slot;       /*slot in global processes*/
    u32     data_len;   /*data len of channel msg*/
    u32     fd_num;     /*bulk tranfer number of fds*/
};

struct msf_channel {
     struct msf_channel_hdr hdr;
     u32    snd_flag;   /*send msg flag*/
     u32    rcv_flag;   /*recv msg flag*/
     u32    cur_pos;    /*bulk tranfer pos of array*/
     u32    *fd_idx;    /*bulk tranfer index of fds*/
     s32    *fd_arr;    /*bulk tranfer array of fds*/
};

struct msf_channel *msf_new_channel(u32 fd_num);
void msf_free_channel(struct msf_channel *ch);
void msf_add_channel(struct msf_channel *ch, s32 fd);
s32 msf_write_channel(s32 fd, struct msf_channel *ch);
s32 msf_read_channel(s32 fd, struct msf_channel *ch);

#endif
