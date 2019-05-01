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

#define MSF_HAVE_CPU_AFFINITY   1
#define MSF_HAVE_GCC_ATOMIC     1
#define MSF_HAVE_SCHED_YIELD    1

#define MSF_HAVE_PREAD          1
#define MSF_HAVE_PWRITE         1

#define MSF_HAVE_F_READAHEAD    0
#define MSF_HAVE_POSIX_FADVISE  1

#define MSF_HAVE_O_DIRECT       1
#define MSF_HAVE_F_NOCACHE      0
#define MSF_HAVE_DIRECTIO       0

#define MSF_HAVE_STATFS         1
#define MSF_HAVE_STATVFS        1

#define MSF_HAVE_FILE_AIO       1
#define MSF_HAVE_MSGHDR_MSG_CONTROL 1
#define MSF_HAVE_SCHED_SETAFFINITY  1

#define MSF_HAVE_LINUX_TIMER        1
#define HAVE_GCC_ATOMICS            1
#define MSF_HAVE_EVENTFD            1
#define MSF_HAVE_SYS_EVENTFD_H      1