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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <butil/logging.h>

#include "Serial.h"

using namespace MSF;

namespace MSF {

int32_t OpenSerialPort(const char *port) {
  int32_t fd = -1;
  // O_NOCTTY If path refers to a terminal device,
  // do not allocate the device as the
  // controlling terminal for this process
  // can use O_NONBLOCK instead of O_NDELAY
  //fd = ::open(port, O_RDWR | O_NOCTTY | O_NDELAY);
  fd = ::open(port, O_RDWR);
  if  (fd < 0) {
    LOG(ERROR) << "Open serial port failed, port: " << port;
    return -1;
  }
  if (::isatty(STDIN_FILENO) == 0) {
    LOG(WARNING) << "Standard input is not a termined device";
  }

  // 设置完属性之后flush
  // set non blocking
  /* 2. read out */
  // uint8_t nr = 0;
	// char buf[64];
  // while (::read(fd, buf, sizeof(buf)) > 0) {
  //   if (++nr == 10) {
  //     nr = 0;
  //     usleep(1);
  //   }
  // }
  // set blocking
  /* 3. restore it to block */
  return fd;
}

// B300 B600 B1200 B2400 B4800 B9600 B19200 B38400 B57600 B115200
int SetSerialBaud(const int fd, const int speed) {
  struct termios opt;
  if (tcgetattr(fd, &opt) < 0) {
    LOG(ERROR) << "Tcgetattr failed, fd: " << fd;
    return -1;
  }
  // memset(&opt, 0, sizeof(opt));
  // opt.c_cflag |= CLOCAL | CREAD;
  // opt.c_cflag &= ~CSIZE;
  cfsetispeed(&opt, speed);
  cfsetospeed(&opt, speed);
  return tcsetattr(fd, TCSANOW, &opt);
}

int SetSerialBits(const int fd, const int bits = 8) {
  struct termios opt;
  if (tcgetattr(fd, &opt) < 0) {
    LOG(ERROR) << "Tcgetattr failed, fd: " << fd;
    return -1;
  }
  switch(bits)
  {
    case 5:
      opt.c_cflag |= CS5;
      break;
    case 6:
      opt.c_cflag |= CS6;
      break;
    case 7:
      opt.c_cflag |= CS7;
      break;
    case 8:
      opt.c_cflag |= CS8;
      break;
    default:
      opt.c_cflag |= CS8;
      break;
  } 
  return tcsetattr(fd, TCSANOW, &opt);
}

struct serl_attr {
	int databits; /* 5 6 7 8 */
	int stopbits; /* 1 or 2 */
	int baudrate; /* 38400, or 9600 , or others. */
	int parity;  /* 0-> no, 1->odd, 2->even */
};
struct serl_attr attr = 
{
  8,1,115200,0,
};

static void __set_parity(struct termios *io, struct serl_attr *attr)
{
	switch (attr->parity) {
		case 0:
			io->c_cflag &= ~PARENB;  /* Clear parity enable */
			io->c_iflag &= ~INPCK;   /* disable parity checking */
			break;
		case 1:
			io->c_cflag |= (PARODD | PARENB); 
			io->c_iflag |= INPCK;    /* odd parity checking */
			break;
		case 2:
			io->c_cflag |= PARENB;   /* Enable parity */
			io->c_cflag &= ~PARODD;
			io->c_iflag |= INPCK;    /*enable parity checking */
			break;
	}
}

int SetSerialEvent(const int fd, const int events = 'N') {
  struct termios opt;
  if (tcgetattr(fd, &opt) < 0) {
    LOG(ERROR) << "Tcgetattr failed, fd: " << fd;
    return -1;
  }
  switch(events)
  {
    case 'O':
    case 'o':
      opt.c_cflag|=PARENB;
      opt.c_cflag|=PARODD;
      opt.c_iflag|=(INPCK|ISTRIP);
      break;		
    case 'E':
    case 'e':
      opt.c_iflag|=(INPCK|ISTRIP);
      opt.c_cflag|=PARENB;
      opt.c_cflag&=~PARODD;
      break;			
    case 'N':
    case 'n':
      opt.c_cflag&=~PARENB;
      break;		
    default:
      opt.c_cflag&=~PARENB;
      break;
  }
  return tcsetattr(fd, TCSANOW, &opt);
}

int SetSerialStop(const int fd, const int stop = 1) {
  struct termios opt;
  if (tcgetattr(fd, &opt) < 0) {
    LOG(ERROR) << "Tcgetattr failed, fd: " << fd;
    return -1;
  }
  switch(stop)
  {
    case 1:
      opt.c_cflag &=~CSTOPB;
      break;
    case 2:
      opt.c_cflag |= CSTOPB;
      break;
    default:
      opt.c_cflag &=~CSTOPB;
      break;
  }
  return tcsetattr(fd, TCSANOW, &opt);
}

int SetSerialRawMode(const int fd) {
  struct termios opt;
  if (tcgetattr(fd, &opt) < 0) {
    LOG(ERROR) << "Tcgetattr failed, fd: " << fd;
    return -1;
  }
  // cfmakeraw(&opt);
  opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); /* input */
  opt.c_iflag &= ~(IXON | IXOFF | ICRNL | INLCR | IGNCR);
  opt.c_oflag &= ~OPOST; /* output */

  opt.c_cc[VTIME] = 2;
  opt.c_cc[VMIN] = 0; // 0 non-block ; > 0 block
  tcflush(fd, TCIFLUSH); 
  // tcflush(fd, TCIOFLUSH);
  // This function returns OK if it was able to perform any of the requested actions, 
  // even if it couldn’t perform all the requested actions. 
  // If the function returns OK, it is our responsibility to
  // see whether all the requested actions were performed. This means that after we call
  // tcsetattr to set the desired attributes, we need to call tcgetattr and compare the
  // actual terminal’s attributes to the desired attributes to detect any differences.
  return tcsetattr(fd, TCSANOW, &opt);
}


void send_break(int fd)
{
	tcsendbreak(fd, 0);
}

// https://blog.csdn.net/yasi_xi/article/details/8246446
/* PeekFd - return amount of data ready to read */
int PeekFd(const int fd) {
  int count;
/*
 * Anticipate a series of system-dependent code fragments.
 */
#ifndef WIN32
  return (ioctl(fd, FIONREAD, (char *)&count) < 0 ? -1 : count);
#else
  return (ioctlsocket(fd, FIONREAD, (unsigned long *)&count) < 0 ? -1 : count);
#endif
}

}  // namespace MSF
