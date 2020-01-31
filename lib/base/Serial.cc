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
#include <base/Serial.h>
#include <sys/ioctl.h>

using namespace MSF::BASE;

namespace MSF {
namespace BASE {

int setSerialBaud(const int fd, const int speed)
{
    struct termios opt;
    tcgetattr(fd,&opt);
    cfsetispeed(&opt,speed);
    cfsetospeed(&opt,speed);
    return tcsetattr(fd, TCSANOW, &opt);
}

int setSerialRawMode(const int fd)
{
    struct termios opt;
    tcgetattr(fd, &opt);
    opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); /* input */
    opt.c_iflag &= ~(IXON|IXOFF|ICRNL|INLCR|IGNCR);
    opt.c_oflag &= ~OPOST;      /* output */
    return tcsetattr(fd,TCSANOW,&opt);
}

//https://blog.csdn.net/yasi_xi/article/details/8246446
/* PeekFd - return amount of data ready to read */
int PeekFd(const int fd)
{
    int count;
    /*
    * Anticipate a series of system-dependent code fragments.
    */
    #ifndef WIN32
    return (ioctl(fd, FIONREAD, (char *) &count) < 0 ? -1 : count);
    #else
    return (ioctlsocket(fd, FIONREAD, (unsigned long *) &count) < 0 ? -1 : count);
    #endif
}


} /**************************** end namespace BASE ****************************/
} /**************************** end namespace MSF  ****************************/

