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
#include <stdio.h>

int plugin_func_stub(const char* func, ...);

/** if func is null, stub func is called
  * and print debug info
  */
#define VA_ARGS(...) , ##__VA_ARGS__
#define FUNCK(func, ...) ((!func)?(plugin_func_stub(__FUNCTION__ VA_ARGS(__VA_ARGS__))):(func(__VA_ARGS__)))

/** when static library not been linked, 
  * this check is needed.
  */
#ifndef ATTRI_CHECK
#define ATTRIBUTE_WIKE  __attribute__((weak))
#else
#define ATTRIBUTE_WIKE
#endif



