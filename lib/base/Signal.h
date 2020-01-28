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
#ifndef __MSF_SIG_H__
#define __MSF_SIG_H__

#include <base/Define.h>
#include <signal.h>

using namespace MSF::BASE;

namespace MSF {
namespace BASE {

typedef void (*SigHandler)(int);
void InitSigHandler();
void RegSigHandler(int sig, SigHandler handler);
void SignalReplace();


} /**************************** end namespace BASE ****************************/
} /**************************** end namespace MSF  ****************************/
#endif
