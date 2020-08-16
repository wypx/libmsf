#include <errno.h>
#include "Mem.h"
#include "UnixSocket.h"

#include <butil/logging.h>

#define HAVE_MSGHDR_MSG_CONTROL 1

using namespace MSF;

/* return bytes# of read on success or negative val on failure. */
int ReadFdMessage(int sockfd, char *buf, int buflen, int *fds, int fd_num) {
  struct iovec iov;
  struct msghdr msgh;
  size_t fdsize = fd_num * sizeof(int);
  char control[CMSG_SPACE(fdsize)];
  struct cmsghdr *cmsg;
  int ret;

  memset(&msgh, 0, sizeof(msgh));
  iov.iov_base = buf;
  iov.iov_len = buflen;

  msgh.msg_iov = &iov;
  msgh.msg_iovlen = 1;
  msgh.msg_control = control;
  msgh.msg_controllen = sizeof(control);

  ret = recvmsg(sockfd, &msgh, MSG_NOSIGNAL | MSG_WAITALL);
  if (ret <= 0) {
    LOG(ERROR) << "Recvmsg failed";
    return ret;
  }

  if (msgh.msg_flags & (MSG_TRUNC | MSG_CTRUNC)) {
    LOG(ERROR) << "Truncted msg";
    return -1;
  }

  for (cmsg = CMSG_FIRSTHDR(&msgh); cmsg != NULL;
       cmsg = CMSG_NXTHDR(&msgh, cmsg)) {
    if (cmsg->cmsg_len < (socklen_t)CMSG_LEN(sizeof(int))) {
      LOG(ERROR) << "Recvmsg return too small ancillary data";
      return -1;
    }
    if ((cmsg->cmsg_level == SOL_SOCKET) && (cmsg->cmsg_type == SCM_RIGHTS)) {
      memcpy(fds, CMSG_DATA(cmsg), fdsize);
      break;
    } else {
      LOG(ERROR) << "Recvmsg return invalid ancillary data level "
                << cmsg->cmsg_level << "or type" << cmsg->cmsg_type;
      return -1;
    }
  }
  return ret;
}

int SendFdMessage(int sockfd, char *buf, int buflen, int *fds, int fd_num) {
  struct iovec iov;
  struct msghdr msgh;
  size_t fdsize = fd_num * sizeof(int);
  char control[CMSG_SPACE(fdsize)];
  struct cmsghdr *cmsg;
  int ret;

  memset(&msgh, 0, sizeof(msgh));
  iov.iov_base = buf;
  iov.iov_len = buflen;

  msgh.msg_iov = &iov;
  msgh.msg_iovlen = 1;

  if (fds && fd_num > 0) {
#ifdef HAVE_MSGHDR_MSG_CONTROL
    msgh.msg_control = control;
    msgh.msg_controllen = sizeof(control);
    cmsg = CMSG_FIRSTHDR(&msgh);
    if (cmsg == NULL) {
      LOG(ERROR) << "cmsg == NULL";
      errno = EINVAL;
      return -1;
    }
    /* CMSG_LEN = CMSG_SPACE + sizeof(struct cmsghdr)
     * Use as:
     *  union {
     *    struct cmsghdr cm;
     *    s8 space[CMSG_SPACE(sizeof(s32)*fd_num)];
     *  } cmsg;
     */
    cmsg->cmsg_len = CMSG_LEN(fdsize);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    /*
     * We have to use memcpy() instead of simple
     * *(int *) CMSG_DATA(&cmsg.cm) = ch->fd;
     * because some gcc 4.4 with -O2/3/s optimization issues the warning:
     * dereferencing type-punned pointer will break strict-aliasing rules
     * Fortunately, gcc with -O1 compiles this msf_memcpy()
     * in the same simple assignment as in the code above
     */
    memcpy(CMSG_DATA(cmsg), fds, CMSG_SPACE(fdsize));
#else
    /* do not support muti transfer socket rights now*/
    cmsg.msg_accrights = (caddr_t)&fds[0];
    cmsg.msg_accrightslen = sizeof(int);
#endif

  } else {
    msgh.msg_control = NULL;
    msgh.msg_controllen = 0;
  }

  do {
    ret = sendmsg(sockfd, &msgh, MSG_NOSIGNAL | MSG_WAITALL);
  } while (ret < 0 && (errno == EINTR || errno == EAGAIN));

  if (ret < 0) {
    LOG(ERROR) << "Sendmsg error";
    return ret;
  }

  return ret;
}