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
#ifndef __MSF_CMSG_H__
#define __MSF_CMSG_H__

#include <stdint.h>
#include <unistd.h>

#define MSF_CMD_OPEN_CHANNEL 1
#define MSF_CMD_CLOSE_CHANNEL 2
#define MSF_CMD_QUIT 3
#define MSF_CMD_TERMINATE 4
#define MSF_CMD_REOPEN 5

#if (__FreeBSD__) && (__FreeBSD_version < 400017)
#include <sys/param.h> /* ALIGN() */

struct cmsghdr {
  socklen_t cmsg_len;
  int cmsg_level;
  int cmsg_type;
  /* u_char     cmsg_data[]; */
};

/*
 * FreeBSD 3.x has no CMSG_SPACE() and CMSG_LEN()
 * and has the broken CMSG_DATA()
 */
#undef CMSG_SPACE
#define CMSG_SPACE(l) (MSF_ALIGN(sizeof(struct cmsghdr)) + MSF_ALIGN(l))

#undef CMSG_LEN
#define CMSG_LEN(l) (MSF_ALIGN(sizeof(struct cmsghdr)) + (l))

#undef CMSG_DATA
#define CMSG_DATA(cmsg) ((uint8_t *)(cmsg) + MSF_ALIGN(sizeof(struct cmsghdr)))
#endif

struct msf_channel_hdr {
  char cmd;
  pid_t pid;
  uint32_t slot;     /*slot in global processes*/
  uint32_t data_len; /*data len of channel msg*/
  uint32_t fd_num;   /*bulk tranfer number of fds*/
};

struct msf_channel {
  struct msf_channel_hdr hdr;
  uint32_t snd_flag; /*send msg flag*/
  uint32_t rcv_flag; /*recv msg flag*/
  uint32_t cur_pos;  /*bulk tranfer pos of array*/
  uint32_t *fd_idx;  /*bulk tranfer index of fds*/
  int *fd_arr;       /*bulk tranfer array of fds*/
};

struct msf_channel *msf_new_channel(uint32_t fd_num);
void msf_free_channel(struct msf_channel *ch);
void msf_add_channel(struct msf_channel *ch, int fd);
int msf_write_channel(int fd, struct msf_channel *ch);
int msf_read_channel(int fd, struct msf_channel *ch);

#endif