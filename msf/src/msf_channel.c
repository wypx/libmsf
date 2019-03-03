
#include <msf_channel.h>

s32 msf_write_channel(s32 fd, struct msf_channel *ch, size_t size) {

    ssize_t n;
    s32     err;
    struct iovec 	iov[1];
    struct msghdr 	msg;

#if (MSF_HAVE_MSGHDR_MSG_CONTROL)

    union {
        struct cmsghdr  cm;
        s8            space[CMSG_SPACE(sizeof(s32))];
    } cmsg;

    if (ch->fd == -1) {
        msg.msg_control = NULL;
        msg.msg_controllen = 0;

    } else {
        msg.msg_control = (caddr_t) &cmsg;
        msg.msg_controllen = sizeof(cmsg);

        msf_memzero(&cmsg, sizeof(cmsg));

        cmsg.cm.cmsg_len = CMSG_LEN(sizeof(s32));
        cmsg.cm.cmsg_level = SOL_SOCKET;
        cmsg.cm.cmsg_type = SCM_RIGHTS;

        /*
         * We have to use memcpy() instead of simple
         *   *(int *) CMSG_DATA(&cmsg.cm) = ch->fd;
         * because some gcc 4.4 with -O2/3/s optimization issues the warning:
         *   dereferencing type-punned pointer will break strict-aliasing rules
         *
         * Fortunately, gcc with -O1 compiles this ngx_memcpy()
         * in the same simple assignment as in the code above
         */

        memcpy(CMSG_DATA(&cmsg.cm), &ch->fd, sizeof(s32));
    }

    msg.msg_flags = 0;

#else

    if (ch->fd == -1) {
        msg.msg_accrights = NULL;
        msg.msg_accrightslen = 0;

    } else {
        msg.msg_accrights = (caddr_t) &ch->fd;
        msg.msg_accrightslen = sizeof(s32);
    }

#endif

    iov[0].iov_base = (char *) ch;
    iov[0].iov_len = size;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    n = sendmsg(fd, &msg, 0);

    if (n == -1) {
        err = errno;
        if (err == EAGAIN) {
            return EAGAIN;
        }
        return -1;
    }

    return 0;
}


s32 msf_read_channel(s32 fd, struct msf_channel *ch, size_t size) {

	ssize_t n;
    s32     err;
    struct iovec 	iov[1];
    struct msghdr 	msg;

#if (MSF_HAVE_MSGHDR_MSG_CONTROL)
	union {
		struct cmsghdr	cm;
		s8			space[CMSG_SPACE(sizeof(s32))];
	} cmsg;
#else
	s32 				fd;
#endif

	iov[0].iov_base = (char *) ch;
	iov[0].iov_len = size;

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

#if (MSF_HAVE_MSGHDR_MSG_CONTROL)
	msg.msg_control = (caddr_t) &cmsg;
	msg.msg_controllen = sizeof(cmsg);
#else
	msg.msg_accrights = (caddr_t) &fd;
	msg.msg_accrightslen = sizeof(int);
#endif

	n = recvmsg(fd, &msg, 0);

	if (n == -1) {
		err = errno;
		if (err == EAGAIN) {
			return EAGAIN;
		}
		return -1;
	}

	if (n == 0) {
		return -1;
	}

	if ((size_t) n < sizeof(struct msf_channel)) {
		printf("recvmsg() returned not enough data: %ld\n", n);
		return -1;
	}

#if (MSF_HAVE_MSGHDR_MSG_CONTROL)

	if (ch->cmd == MSF_CMD_OPEN_CHANNEL) {

		if (cmsg.cm.cmsg_len < (socklen_t) CMSG_LEN(sizeof(int))) {
			printf("recvmsg() returned too small ancillary data");
			return -1;
		}

		if (cmsg.cm.cmsg_level != SOL_SOCKET || cmsg.cm.cmsg_type != SCM_RIGHTS)
		{
			printf("recvmsg() returned invalid ancillary data "
						  "level %d or type %d\n",
						  cmsg.cm.cmsg_level, cmsg.cm.cmsg_type);
			return -1;
		}

		/* ch->fd = *(int *) CMSG_DATA(&cmsg.cm); */
		memcpy(&ch->fd, CMSG_DATA(&cmsg.cm), sizeof(int));
	}

	if (msg.msg_flags & (MSG_TRUNC|MSG_CTRUNC)) {
		printf("recvmsg() truncated data\n");
	}

#else

	if (ch->cmd == MSF_CMD_OPEN_CHANNEL) {
		if (msg.msg_accrightslen != sizeof(s32)) {
			printf("recvmsg() returned no ancillary data\n");
			return -1;
		}

		ch->fd = fd;
	}

#endif

	return n;
}

s32 msf_add_channel_event(s32 fd, s32 event) {

	return 0;
}
void msf_close_channel(s32 *fd) {

	sclose(fd[0]);
	sclose(fd[1]);
}

