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
#ifndef BASE_SERIAL_H_
#define BASE_SERIAL_H_

#include <stdint.h>
#include <termios.h>

namespace MSF {

int32_t OpenSerialPort(const char *port);
int SetSerialBaud(const int fd, const int speed);
int SetSerialRawMode(const int fd);

}  // namespace MSF
#endif