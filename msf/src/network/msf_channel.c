/**************************************************************************
*
* Copyright (c) 2017-2019, luotang.me <wypx520@gmail.com>, China.
* All rights reserved.
*
* Distributed under the terms of the GNU General Public License v2.
*
* This software is provided 'as is' with no explicit or implied warranties
* in respect of its properties, including, but not limited to, correctness
* and/or fitness for purpose.
*
**************************************************************************/
#include <msf_channel.h>

#define MSF_MOD_CHANNEL "CHANNEL"
#define MSF_CHANNEL_LOG(level, ...) \
    msf_log_write(level, MSF_MOD_CHANNEL, MSF_FUNC_FILE_LINE, __VA_ARGS__)

struct msf_channel *msf_new_channel(u32 fd_num) {

    struct msf_channel *ch = NULL;

    ch = MSF_CALLOC_TYPE(1, struct msf_channel, sizeof(struct msf_channel));
    if (ch == NULL) {
         MSF_CHANNEL_LOG(DBG_ERROR, "Malloc channel failed.");
         return NULL;
    }

    ch->fd_idx = MSF_CALLOC_TYPE(fd_num, u32, sizeof(u32));
    if (ch->fd_idx == NULL) {
        sfree(ch);
        MSF_CHANNEL_LOG(DBG_ERROR, "Malloc channel fd index failed.");
        return NULL;
    }

    ch->fd_arr = MSF_CALLOC_TYPE(fd_num, s32, sizeof(s32));
    if (ch->fd_arr == NULL) {
        sfree(ch->fd_idx);
        sfree(ch);
        MSF_CHANNEL_LOG(DBG_ERROR, "Malloc channel fd array failed.");
        return NULL;
    }

    ch->snd_flag = MSG_NOSIGNAL | MSG_WAITALL;
    ch->rcv_flag = MSG_NOSIGNAL | MSG_WAITALL;
    ch->cur_pos = 0;
    ch->hdr.fd_num = fd_num;
    ch->hdr.data_len = fd_num * sizeof(u32);

    return ch;
}

void msf_free_channel(struct msf_channel *ch) {
    sfree(ch->fd_arr);
    sfree(ch->fd_idx);
    sfree(ch);
}

void msf_add_channel(struct msf_channel *ch, s32 fd) {

    if (unlikely(ch->cur_pos >= (ch->hdr.fd_num - 1))) {
        MSF_CHANNEL_LOG(DBG_ERROR, "Except max limit fd num.");
        return;
    }
    ch->fd_arr[ch->cur_pos] = fd;
    ch->cur_pos++;
}

s32 msf_write_channel_hdr(s32 fd, struct msf_channel_hdr *hdr) {

    struct iovec iov[1];

    iov[0].iov_base = (s8 *)hdr;
    iov[0].iov_len = sizeof(struct msf_channel_hdr);

    return msf_sendmsg_v1(fd, iov, 1, MSG_NOSIGNAL | MSG_WAITALL);
}

s32 msf_read_channel_hdr(s32 fd, struct msf_channel_hdr *hdr) {

    struct iovec iov[1];

    iov[0].iov_base = (s8 *)hdr;
    iov[0].iov_len = sizeof(struct msf_channel_hdr);

    return msf_sendmsg_v1(fd, iov, 1, MSG_NOSIGNAL | MSG_WAITALL);
}

s32 msf_write_channel(s32 fd, struct msf_channel *ch) {

    s32     rc;
    s32     err;
    struct iovec    iov[1];
    struct msghdr   msg = { 0 };

#if (MSF_HAVE_MSGHDR_MSG_CONTROL)
    s32 cmsg_len;
    struct cmsghdr *cmsg;

    /* CMSG_LEN = CMSG_SPACE + sizeof(struct cmsghdr)
     * Use as:
     *  union {
     *    struct cmsghdr cm;
     *    s8 space[CMSG_SPACE(sizeof(s32)*fd_num)];
     *  } cmsg;
     */
    cmsg_len = CMSG_LEN(sizeof(s32)*ch->hdr.fd_num);
    cmsg = MSF_CALLOC_TYPE(1, struct cmsghdr, cmsg_len);
    if (cmsg == NULL) {
        MSF_CHANNEL_LOG(DBG_ERROR, "Malloc channel failed.");
        return -1;
    }

    msg.msg_control = (caddr_t) &cmsg;
    msg.msg_controllen = sizeof(cmsg);
    msf_memzero(&cmsg, sizeof(cmsg));

    cmsg->cmsg_len = cmsg_len;
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;

    /*
     * We have to use memcpy() instead of simple
     *   *(int *) CMSG_DATA(&cmsg.cm) = ch->fd;
     * because some gcc 4.4 with -O2/3/s optimization issues the warning:
     *   dereferencing type-punned pointer will break strict-aliasing rules
     *
     * Fortunately, gcc with -O1 compiles this msf_memcpy()
     * in the same simple assignment as in the code above
     */
    msf_memcpy(CMSG_DATA(cmsg), ch->fd_arr,
                CMSG_SPACE(sizeof(s32)*ch->hdr.fd_num));

#else
    /* do not support muti transfer socket rights now*/
    msg.msg_accrights = (caddr_t) &ch->fd_arr[0];
    msg.msg_accrightslen = sizeof(s32);
#endif

    iov[0].iov_base = (s8 *)ch->fd_idx;
    iov[0].iov_len = ch->hdr.data_len;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    rc = msf_sendmsg_v2(fd, &msg, MSG_NOSIGNAL | MSG_WAITALL);
    if (rc == -1) {
        sfree(cmsg);
        err = errno;
        if (err == MSF_EAGAIN) {
            return MSF_EAGAIN;
        }
        return -1;
    }

    sfree(cmsg);

    return 0;
}

s32 msf_read_channel(s32 fd, struct msf_channel *ch) {

    s32     rc;
    s32     err;
    struct iovec    iov[1];
    struct msghdr   msg = { 0 };

#if (MSF_HAVE_MSGHDR_MSG_CONTROL)
    s32 cmsg_len;
    struct cmsghdr *cmsg;

    cmsg_len = CMSG_LEN(sizeof(s32)*ch->hdr.fd_num);
    cmsg = MSF_CALLOC_TYPE(1, struct cmsghdr, cmsg_len);
    if (cmsg == NULL) {
        MSF_CHANNEL_LOG(DBG_ERROR, "Malloc channel failed.");
        return -1;
    }

#else
    s32 fd;
#endif

    iov[0].iov_base = (s8 *)ch->fd_idx;
    iov[0].iov_len = cmsg_len;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

#if (MSF_HAVE_MSGHDR_MSG_CONTROL)
    msg.msg_control = (caddr_t) &cmsg;
    msg.msg_controllen = sizeof(cmsg);
#else
    msg.msg_accrights = (caddr_t) &fd;
    msg.msg_accrightslen = sizeof(s32);
#endif

    rc = msf_recvmsg_v2(fd, &msg, MSG_NOSIGNAL | MSG_WAITALL);
    if (rc == -1) {
        err = errno;
        if (err == MSF_EAGAIN) {
            return MSF_EAGAIN;
        }
        return -1;
    }

    if (rc == 0) {
        return -1;
    }

    if ((size_t) rc < sizeof(struct msf_channel)) {
        MSF_CHANNEL_LOG(DBG_ERROR, "Recvmsg return not enough data: %zd.", rc);
        return -1;
    }

#if (MSF_HAVE_MSGHDR_MSG_CONTROL)
    if (ch->hdr.cmd == MSF_CMD_OPEN_CHANNEL) {
        if (cmsg->cmsg_len < (socklen_t) CMSG_LEN(sizeof(s32))) {
            MSF_CHANNEL_LOG(DBG_ERROR, "Recvmsg return too small ancillary data");
            return -1;
        }

        if (cmsg->cmsg_level != SOL_SOCKET 
            || cmsg->cmsg_type != SCM_RIGHTS)
        {
            MSF_CHANNEL_LOG(DBG_ERROR, "Recvmsg return invalid ancillary data "
                         "level %d or type %d",
                          cmsg->cmsg_level, cmsg->cmsg_type);
            return -1;
        }

        /* ch->fd = *(int *) CMSG_DATA(&cmsg.cm); */
        msf_memcpy((s8*)ch->fd_arr, CMSG_DATA(cmsg),
                     sizeof(s32)*ch->hdr.fd_num);
    }

    if (msg.msg_flags & (MSG_TRUNC|MSG_CTRUNC)) {
        MSF_CHANNEL_LOG(DBG_ERROR, "recvmsg() truncated data");
    }

#else

    if (ch->hdr.cmd == MSF_CMD_OPEN_CHANNEL) {
        if (msg.msg_accrightslen != sizeof(s32)) {
            MSF_CHANNEL_LOG(DBG_ERROR, "recvmsg() returned no ancillary data");
            return -1;
        }

        ch->fd_arr[0] = fd;
    }

#endif

    return rc;
}

