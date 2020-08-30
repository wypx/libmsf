/**************************************************************************
 *
 * Copyright (c) 2017-2021, luotang.me <wypx520@gmail.com>, China.
 * All rights reserved.
 *
 * Distributed under the terms of the GNU General Public License v2.
 *
 * This software is provided 'as is' with no explicit or implied warranties
 * in respect of its properties, including, but not limited to, correctneLOG(ERROR)
 * and/or fitneLOG(ERROR) for purpose.
 *
 **************************************************************************/
#include "Serial.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#if defined(__linux__)
#include <linux/serial.h>
#endif

#include <butil/logging.h>

using namespace MSF;

bool xonxoff_;
bool rtscts_;

namespace MSF {

int32_t OpenSerialPort(const char *port) {
  int32_t fd = -1;
  // O_NOCTTY If path refers to a terminal device,
  // do not allocate the device as the
  // controlling terminal for this proceLOG(ERROR)
  // can use O_NONBLOCK instead of O_NDELAY
  // fd = ::open(port, O_RDWR | O_NOCTTY | O_NDELAY);
  fd = ::open(port, O_RDWR);
  if (fd < 0) {
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

  // https://www.cnblogs.com/schips/p/10265178.html
  #if defined(__linux__) && defined (TIOCSSERIAL)
  struct serial_struct ser;

  if (::ioctl (fd, TIOCGSERIAL, &ser) < 0) {
    return -1;
  }

  // set custom divisor
  ser.custom_divisor = ser.baud_base / static_cast<int>(speed);
  // update flags
  ser.flags &= ~ASYNC_SPD_MASK;
  ser.flags |= ASYNC_SPD_CUST;

  if (::ioctl (fd, TIOCSSERIAL, &ser) < 0) {
    return -1;
  }
  #endif

  // set up raw mode / no echo / binary
  // memset(&opt, 0, sizeof(opt));
  // opt.c_cflag |= CLOCAL | CREAD;
  // opt.c_cflag &= ~CSIZE;
  cfsetispeed(&opt, speed);
  cfsetospeed(&opt, speed);

  return tcsetattr(fd, TCSANOW, &opt);
}



int SetSerialBits(const int fd, SerialByteSize bytesize = EightBits) {
  struct termios opt;
  if (tcgetattr(fd, &opt) < 0) {
    LOG(ERROR) << "Tcgetattr failed, fd: " << fd;
    return -1;
  }
  // setup char len
  opt.c_cflag &= (tcflag_t) ~CSIZE;
  if (bytesize == EightBits)
    opt.c_cflag |= CS8;
  else if (bytesize == SevenBits)
    opt.c_cflag |= CS7;
  else if (bytesize == SixBits)
    opt.c_cflag |= CS6;
  else if (bytesize == FiveBits)
    opt.c_cflag |= CS5;

  return tcsetattr(fd, TCSANOW, &opt);
}

struct serl_attr {
  int databits; /* 5 6 7 8 */
  int stopbits; /* 1 or 2 */
  int baudrate; /* 38400, or 9600 , or others. */
  int parity;   /* 0-> no, 1->odd, 2->even */
};
struct serl_attr attr = {
    8,
    1,
    115200,
    0,
};

void SetParity(struct termios *options, SerialParity parity = ParityNone) {
  options->c_iflag &= (tcflag_t) ~(INPCK | ISTRIP);
  switch (parity) {
    case ParityNone: {
        options->c_cflag &= (tcflag_t) ~(PARENB | PARODD);/* Clear parity enable */
        options->c_iflag &= ~INPCK;  /* disable parity checking */
      }
      break;
    case ParityOdd: {
        options->c_cflag |=  (PARENB | PARODD);
        options->c_iflag |= INPCK; /* odd parity checking */
      }
      break;
    case ParityEven: {
        options->c_cflag &= (tcflag_t) ~(PARODD);
        options->c_cflag |=  (PARENB); /* Enable parity */
        options->c_iflag |= INPCK; /*enable parity checking */
      }
      break;
    default:
      break;
  }
}

int SetSerialEvent(const int fd, const int events = 'N') {
  struct termios opt;
  if (tcgetattr(fd, &opt) < 0) {
    LOG(ERROR) << "Tcgetattr failed, fd: " << fd;
    return -1;
  }
  switch (events) {
    case 'O':
    case 'o':
      opt.c_cflag |= PARENB;
      opt.c_cflag |= PARODD;
      opt.c_iflag |= (INPCK | ISTRIP);
      break;
    case 'E':
    case 'e':
      opt.c_iflag |= (INPCK | ISTRIP);
      opt.c_cflag |= PARENB;
      opt.c_cflag &= ~PARODD;
      break;
    case 'N':
    case 'n':
      opt.c_cflag &= ~PARENB;
      break;
    default:
      opt.c_cflag &= ~PARENB;
      break;
  }
  return tcsetattr(fd, TCSANOW, &opt);
}

int SetSerialStop(const int fd, const SerialStopBits stop = StopBitsOne) {
  struct termios opt;
  if (tcgetattr(fd, &opt) < 0) {
    LOG(ERROR) << "Tcgetattr failed, fd: " << fd;
    return -1;
  }

  switch (stop) {
    case StopBitsOne:
      opt.c_cflag &= ~CSTOPB;
      break;
    case StopBitsTwo:
    // ONE POINT FIVE same as TWO.. there is no POSIX support for 1.5
    case StopBitsOnePointFive:
      opt.c_cflag |= CSTOPB;
      break;
    default:
      opt.c_cflag &= ~CSTOPB;
      break;
  }
  return tcsetattr(fd, TCSANOW, &opt);
}

int SetSerialRawMode(const int fd, SerialFlowControl flowcontrol) {
  struct termios opt;
  if (tcgetattr(fd, &opt) < 0) {
    LOG(ERROR) << "Tcgetattr failed, fd: " << fd;
    return -1;
  }
  // cfmakeraw(&opt);
  opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | ECHOK | ECHONL | IEXTEN); /* input ECHOPRT */
  opt.c_iflag &= ~(IXON | IXOFF | ICRNL | INLCR | IGNCR | IGNBRK);
  opt.c_oflag &= ~OPOST; /* output */

#ifdef IUCLC
  opt.c_iflag &= (tcflag_t) ~IUCLC;
#endif
#ifdef PARMRK
  opt.c_iflag &= (tcflag_t) ~PARMRK;
#endif

 // setup flow control
  if (flowcontrol == FlowControlNone) {
    xonxoff_ = false;
    rtscts_ = false;
  }
  if (flowcontrol == FlowControlSoftware) {
    xonxoff_ = true;
    rtscts_ = false;
  }
  if (flowcontrol == FlowControlHardware) {
    xonxoff_ = false;
    rtscts_ = true;
  }
  // xonxoff
#ifdef IXANY
  if (xonxoff_)
    opt.c_iflag |=  (IXON | IXOFF); //|IXANY)
  else
    opt.c_iflag &= (tcflag_t) ~(IXON | IXOFF | IXANY);
#else
  if (xonxoff_)
    opt.c_iflag |=  (IXON | IXOFF);
  else
    opt.c_iflag &= (tcflag_t) ~(IXON | IXOFF);
#endif
  // rtscts
#ifdef CRTSCTS
  if (rtscts_)
    opt.c_cflag |=  (CRTSCTS);
  else
    opt.c_cflag &= (unsigned long) ~(CRTSCTS);
#elif defined CNEW_RTSCTS
  if (rtscts_)
    opt.c_cflag |=  (CNEW_RTSCTS);
  else
    opt.c_cflag &= (unsigned long) ~(CNEW_RTSCTS);
#else
#error "OS Support seems wrong."
#endif

  // http://www.unixwiz.net/techtips/termios-vmin-vtime.html
  // this basically sets the read call up to be a polling read,
  // but we are using select to ensure there is data available
  // to read before each call, so we should never needleLOG(ERROR)ly poll
  opt.c_cc[VTIME] = 2;
  opt.c_cc[VMIN] = 0;  // 0 non-block ; > 0 block
  tcflush(fd, TCIFLUSH);
  // tcflush(fd, TCIOFLUSH);
  // This function returns OK if it was able to perform any of the requested
  // actions, even if it couldn’t perform all the requested actions. If the
  // function returns OK, it is our responsibility to see whether all the
  // requested actions were performed. This means that after we call tcsetattr
  // to set the desired attributes, we need to call tcgetattr and compare the
  // actual terminal’s attributes to the desired attributes to detect any
  // differences.
  return tcsetattr(fd, TCSANOW, &opt);
}

// https://blog.csdn.net/yasi_xi/article/details/8246446
/* PeekFd - return amount of data ready to read */
int PeekFd(const int fd) {
  int count;
/*
 * Anticipate a series of system-dependent code fragments.
 */
#ifndef WIN32
  return (::ioctl(fd, FIONREAD, (char *)&count) < 0 ? -1 : count);
#else
  return (ioctlsocket(fd, FIONREAD, (unsigned long *)&count) < 0 ? -1 : count);
#endif
}

void FlushInput(const int fd)
{
  ::tcflush(fd, TCIFLUSH);
}

void FlushOutput(const int fd)
{
  ::tcflush(fd, TCOFLUSH);
}

void SendBreak(const int fd, int duration)
{
  ::tcsendbreak(fd, static_cast<int>(duration / 4));
}

void SetBreak(const int fd, bool level) {
  if (level) {
    if (::ioctl(fd, TIOCSBRK) < 0) {
      LOG(ERROR) << "setBreak failed on a call to ioctl(TIOCSBRK): " << errno;
    }
  } else {
    if (::ioctl(fd, TIOCCBRK) <  0){
      LOG(ERROR) << "setBreak failed on a call to ioctl(TIOCCBRK): " << errno;
    }
  }
}

void SetRTS(const int fd, bool level) {
  int command = TIOCM_RTS;
  if (level) {
    if (::ioctl(fd, TIOCMBIS, &command) < 0) {
      LOG(ERROR) << "setRTS failed on a call to ioctl(TIOCMBIS): " << errno;
    }
  } else {
    if (::ioctl(fd, TIOCMBIC, &command) <  0) {
      LOG(ERROR) << "setRTS failed on a call to ioctl(TIOCMBIC): " << errno;
    }
  }
}

void SetDTR(const int fd, bool level) {
  int command = TIOCM_DTR;
  if (level) {
    if (::ioctl (fd, TIOCMBIS, &command) < 0) {
      LOG(ERROR) << "setDTR failed on a call to ioctl(TIOCMBIS): " << errno;
    }
  } else {
    if (::ioctl (fd, TIOCMBIC, &command) < 0) {
      LOG(ERROR) << "setDTR failed on a call to ioctl(TIOCMBIC): " << errno;
    }
  }
}


bool WaitForChange (const int fd, bool is_open) {
#ifndef TIOCMIWAIT
  while (is_open == true) {
    int status;
    if (::ioctl(fd, TIOCMGET, &status) < 0) {
      LOG(ERROR) << "waitForChange failed on a call to ioctl(TIOCMGET): " << errno;
    } else {
      if (0 != (status & TIOCM_CTS)
        || 0 != (status & TIOCM_DSR)
        || 0 != (status & TIOCM_RI)
        || 0 != (status & TIOCM_CD)) {
        return true;
      }
    }
    usleep(1000);
  }
  return false;
#else
  int command = (TIOCM_CD|TIOCM_DSR|TIOCM_RI|TIOCM_CTS);
  if (::ioctl (fd, TIOCMIWAIT, &command) < 0) {
    LOG(ERROR) << "waitForDSR failed on a call to ioctl(TIOCMIWAIT): " << errno;
  }
  return true;
#endif
}

bool GetCTS(const int fd)
{
  int status;
  if (::ioctl (fd, TIOCMGET, &status) < 0) {
    LOG(ERROR) << "GetCTS failed on a call to ioctl(TIOCMGET): " << errno;
  } else {
    return 0 != (status & TIOCM_CTS);
  }
}

bool GetDSR(const int fd)
{
  int status;
  if (::ioctl(fd, TIOCMGET, &status) < 0) {
    LOG(ERROR) << "GetDSR failed on a call to ioctl(TIOCMGET): "<< errno;
  } else {
    return 0 != (status & TIOCM_DSR);
  }
}

bool GetRI(const int fd)
{
  int status;
  if (::ioctl(fd, TIOCMGET, &status) < 0) {
    LOG(ERROR) << "getRI failed on a call to ioctl(TIOCMGET): " << errno;
  } else {
    return 0 != (status & TIOCM_RI);
  }
}

bool GetCD(const int fd) {
  int status;
  if (::ioctl (fd, TIOCMGET, &status) < 0) {
    LOG(ERROR) << "getCD failed on a call to ioctl(TIOCMGET): " << errno;
  } else {
    return 0 != (status & TIOCM_CD);
  }
}


size_t Available(const int fd) {
  int count = 0;
  if (::ioctl (fd, TIOCINQ, &count) < 0) {
    return -1;
  } else {
    return static_cast<size_t>(count);
  }
}

}  // namespace MSF
