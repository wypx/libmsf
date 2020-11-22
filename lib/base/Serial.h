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

// Enumeration defines the possible bytesizes for the serial port.
typedef enum {
  FiveBits = 5,
  SixBits = 6,
  SevenBits = 7,
  EightBits = 8
} SerialByteSize;

// Enumeration defines the possible flowcontrol types for the serial port.
typedef enum {
  FlowControlNone = 0,
  FlowControlSoftware,
  FlowControlHardware
} SerialFlowControl;

// Enumeration defines the possible parity types for the serial port.
typedef enum {
  ParityNone = 0,
  ParityOdd = 1,
  ParityEven = 2,
  ParityMark = 3,
  ParitySpace = 4
} SerialParity;

// Enumeration defines the possible stopbit types for the serial port.
typedef enum {
  StopBitsOne = 1,
  StopBitsTwo = 2,
  StopBitsOnePointFive
} SerialStopBits;

int32_t OpenSerialPort(const char *port);
int SetSerialBaud(const int fd, const int speed);
int SetSerialRawMode(const int fd,
                     SerialFlowControl flowcontrol = FlowControlHardware);

}  // namespace MSF
#endif