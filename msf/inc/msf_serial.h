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
#include <msf_utils.h>
#include <termios.h> 

#ifndef __MSF_SERIAL_H__
#define __MSF_SERIAL_H__ 

s32 msf_serial_baud(s32 fd, s32 speed);

s32 msf_serial_rawmode(s32 fd);

#endif
