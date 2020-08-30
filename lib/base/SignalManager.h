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
#ifndef LIB_SIGNAL_H_
#define LIB_SIGNAL_H_

#include <signal.h>

namespace MSF {

typedef void (*SigHandler)(int);
void InitSigHandler();
void RegSigHandler(int sig, SigHandler handler);
void SignalReplace();

}  // namespace MSF
#endif
