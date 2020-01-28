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
#ifndef __MSF_DAEMON_H__
#define __MSF_DAEMON_H__

#include <base/Define.h>

using namespace MSF::BASE;

namespace MSF {
namespace BASE {

typedef void (*MSF_SIGHAND_T)(int);

class MSF_DAEMON {
    public:
        static int msf_redirect_output(const char *dev, int *fd_output, int *fd_stdout);
        static int msf_redirect_output_cancle(int* fd_output, int *fd_stdout);
        static int msf_daemonize(bool nochdir, bool noclose);
        static void msf_close_std_fds(void);
        static int daemonize(bool nochdir, bool noclose);
        // static int msf_daemonize(bool nochdir, bool noclose);
};

} /**************************** end namespace BASE ****************************/
} /**************************** end namespace MSF  ****************************/
#endif

