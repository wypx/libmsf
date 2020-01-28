#include <base/Logger.h>
#include <base/Mem.h>
#include <sock/UnixCmsg.h>
#include <errno.h>

#define MSF_HAVE_MSGHDR_MSG_CONTROL 1

using namespace MSF::BASE;

struct msf_channel *msf_new_channel(uint32_t fd_num) {

    struct msf_channel *ch = NULL;

    ch = (struct msf_channel*)MSF_CALLOC_TYPE(1, struct msf_channel, sizeof(struct msf_channel));
    if (ch == NULL) {
         MSF_ERROR << "Malloc channel failed.";
         return NULL;
    }

    ch->fd_idx = (uint32_t*)MSF_CALLOC_TYPE(fd_num, uint32_t, sizeof(uint32_t));
    if (ch->fd_idx == NULL) {
        free(ch);
        MSF_ERROR << "Malloc channel fd index failed.";
        return NULL;
    }

    ch->fd_arr = (int*)MSF_CALLOC_TYPE(fd_num, int, sizeof(int));
    if (ch->fd_arr == NULL) {
        free(ch->fd_idx);
        free(ch);
        MSF_ERROR << "Malloc channel fd array failed.";
        return NULL;
    }

    ch->snd_flag = MSG_NOSIGNAL | MSG_WAITALL;
    ch->rcv_flag = MSG_NOSIGNAL | MSG_WAITALL;
    ch->cur_pos = 0;
    ch->hdr.fd_num = fd_num;
    ch->hdr.data_len = fd_num * sizeof(uint32_t);

    return ch;
}

void msf_free_channel(struct msf_channel *ch) {
    free(ch->fd_arr);
    free(ch->fd_idx);
    free(ch);
}

void msf_add_channel(struct msf_channel *ch, int fd) {

    if (unlikely(ch->cur_pos >= (ch->hdr.fd_num - 1))) {
        MSF_ERROR << "Except max limit fd num.";
        return;
    }
    ch->fd_arr[ch->cur_pos] = fd;
    ch->cur_pos++;
}

int msf_write_channel_hdr(int fd, struct msf_channel_hdr *hdr)
{
    struct msghdr msg;
    struct iovec iov[1];
    iov[0].iov_base = (char *)hdr;
    iov[0].iov_len = sizeof(struct msf_channel_hdr);
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    return sendmsg(fd, &msg, MSG_NOSIGNAL | MSG_WAITALL);
}

int msf_read_channel_hdr(int fd, struct msf_channel_hdr *hdr) 
{
    struct msghdr msg;
    struct iovec iov[1];

    iov[0].iov_base = (char *)hdr;
    iov[0].iov_len = sizeof(struct msf_channel_hdr);
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    return sendmsg(fd, &msg, MSG_NOSIGNAL | MSG_WAITALL);
}

int msf_write_channel(int fd, struct msf_channel *ch) 
{
    int     rc;
    int     err;
    struct iovec    iov[1];
    struct msghdr   msg = { 0 };

#if (MSF_HAVE_MSGHDR_MSG_CONTROL)
    int cmsg_len;
    struct cmsghdr *cmsg;

    /* CMSG_LEN = CMSG_SPACE + sizeof(struct cmsghdr)
     * Use as:
     *  union {
     *    struct cmsghdr cm;
     *    s8 space[CMSG_SPACE(sizeof(s32)*fd_num)];
     *  } cmsg;
     */
    cmsg_len = CMSG_LEN(sizeof(int)*ch->hdr.fd_num);
    cmsg = MSF_CALLOC_TYPE(1, struct cmsghdr, cmsg_len);
    if (cmsg == NULL) {
        MSF_ERROR << "Malloc channel failed.";
        return -1;
    }

    msg.msg_control = (caddr_t) &cmsg;
    msg.msg_controllen = sizeof(cmsg);
    memset(&cmsg, 0, sizeof(cmsg));

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
    memcpy(CMSG_DATA(cmsg), ch->fd_arr,
                CMSG_SPACE(sizeof(int)*ch->hdr.fd_num));

#else
    /* do not support muti transfer socket rights now*/
    msg.msg_accrights = (caddr_t) &ch->fd_arr[0];
    msg.msg_accrightslen = sizeof(int);
#endif

    iov[0].iov_base = (char *)ch->fd_idx;
    iov[0].iov_len = ch->hdr.data_len;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    rc = sendmsg(fd, &msg, MSG_NOSIGNAL | MSG_WAITALL);
    if (rc == -1) {
        free(cmsg);
        err = errno;
        if (err == EAGAIN) {
            return EAGAIN;
        }
        return -1;
    }

    free(cmsg);

    return 0;
}

int msf_read_channel(int fd, struct msf_channel *ch) {

    int     rc;
    int     err;
    struct iovec    iov[1];
    struct msghdr   msg = { 0 };

#if (MSF_HAVE_MSGHDR_MSG_CONTROL)
    int cmsg_len;
    struct cmsghdr *cmsg;

    cmsg_len = CMSG_LEN(sizeof(int)*ch->hdr.fd_num);
    cmsg = MSF_CALLOC_TYPE(1, struct cmsghdr, cmsg_len);
    if (cmsg == NULL) {
        MSF_ERROR << "Malloc channel failed.";
        return -1;
    }

#else
    int fd;
#endif

    iov[0].iov_base = (char *)ch->fd_idx;
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

    rc = recvmsg(fd, &msg, MSG_NOSIGNAL | MSG_WAITALL);
    if (rc == -1) {
        err = errno;
        if (err == EAGAIN) {
            return EAGAIN;
        }
        return -1;
    }

    if (rc == 0) {
        return -1;
    }

    if ((size_t) rc < sizeof(struct msf_channel)) {
        MSF_ERROR << "Recvmsg return not enough data: " << rc;
        return -1;
    }

#if (MSF_HAVE_MSGHDR_MSG_CONTROL)
    if (ch->hdr.cmd == MSF_CMD_OPEN_CHANNEL) {
        if (cmsg->cmsg_len < (socklen_t) CMSG_LEN(sizeof(int))) {
            MSF_ERROR << "Recvmsg return too small ancillary data";
            return -1;
        }

        if (cmsg->cmsg_level != SOL_SOCKET 
            || cmsg->cmsg_type != SCM_RIGHTS)
        {
            MSF_ERROR << "Recvmsg return invalid ancillary data level "
                        << cmsg->cmsg_level << "or type" << cmsg->cmsg_type;
            return -1;
        }

        /* ch->fd = *(int *) CMSG_DATA(&cmsg.cm); */
        memcpy((char*)ch->fd_arr, CMSG_DATA(cmsg),
                     sizeof(int)*ch->hdr.fd_num);
    }

    if (msg.msg_flags & (MSG_TRUNC|MSG_CTRUNC)) {
        MSF_ERROR << "recvmsg() truncated data";
    }

#else

    if (ch->hdr.cmd == MSF_CMD_OPEN_CHANNEL) {
        if (msg.msg_accrightslen != sizeof(s32)) {
            MSF_ERROR << "recvmsg() returned no ancillary data";
            return -1;
        }

        ch->fd_arr[0] = fd;
    }

#endif

    return rc;
}

