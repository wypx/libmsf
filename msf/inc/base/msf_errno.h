/**************************************************************************
*
* Copyright (c) 2017-2020, luotang.me <wypx520@gmail.com>, China.
* All rights reserved.
*
* Distributed under the terms of the GNU General Public License v2.
*
* This software is provided 'as is' with no explicit or implied warranties
* in respect of its properties, including, but not limited to, correctness
* and/or fitness for purpose.
*
**************************************************************************/

#ifndef __MSF_ERRNO_H__
#define __MSF_ERRNO_H__

#include <errno.h>

#define MSF_EPERM         EPERM
#define MSF_ENOENT        ENOENT //参数file_name指定的文件不存在
#define MSF_ENOPATH       ENOENT
#define MSF_ESRCH         ESRCH
#define MSF_EINTR         EINTR
#define MSF_ECHILD        ECHILD
#define MSF_ENOMEM        ENOMEM
#define MSF_EACCES        EACCES
#define MSF_EBUSY         EBUSY
#define MSF_EEXIST        EEXIST
#define MSF_EXDEV         EXDEV
#define MSF_ENOTDIR       ENOTDIR
#define MSF_EISDIR        EISDIR
#define MSF_EINVAL        EINVAL
#define MSF_ENFILE        ENFILE
#define MSF_EMFILE        EMFILE
#define MSF_ENOSPC        ENOSPC
#define MSF_EPIPE         EPIPE
#define MSF_ESHUTDOWN     ESHUTDOWN
#define MSF_EINPROGRESS   EINPROGRESS  /* connect on non-blocking socket */
#define MSF_ENOPROTOOPT   ENOPROTOOPT
#define MSF_EOPNOTSUPP    EOPNOTSUPP
#define MSF_EADDRINUSE    EADDRINUSE
#define MSF_ECONNABORTED  ECONNABORTED
#define MSF_ECONNRESET    ECONNRESET
#define MSF_ENOTCONN      ENOTCONN
#define MSF_ETIMEDOUT     ETIMEDOUT
#define MSF_ECONNREFUSED  ECONNREFUSED
#define MSF_ENAMETOOLONG  ENAMETOOLONG
#define MSF_ENETDOWN      ENETDOWN
#define MSF_ENETUNREACH   ENETUNREACH
#define MSF_EHOSTDOWN     EHOSTDOWN
#define MSF_EHOSTUNREACH  EHOSTUNREACH
#define MSF_ENOSYS        ENOSYS
#define MSF_ECANCELED     ECANCELED
#define MSF_EILSEQ        EILSEQ
#define MSF_ENOMOREFILES  0
#define MSF_ELOOP         ELOOP
#define MSF_EBADF         EBADF
#define MSF_EMLINK        EMLINK

#define MSF_EAGAIN        EAGAIN      /* recv on non-blocking socket */
#define MSF_EWOULDBLOCK   EWOULDBLOCK /* recv on non-blocking socket */

#define MSF_EADDRNOTAVAIL EADDRNOTAVAIL

#define msf_errno                  errno
#define msf_socket_errno           errno
#define msf_set_errno(err)         errno = err
#define msf_set_socket_errno(err)  errno = err

#endif
