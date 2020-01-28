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
#ifndef __MSF_DEF_H__
#define __MSF_DEF_H__

namespace MSF {
namespace BASE {

#define MSF_OK                  (0)
#define MSF_ERR                 (-1)

#define MSF_NONE_WAIT           (0)
#define MSF_WAIT_FOREVER        (-1)

#define MSF_INVALID_SOCKET      (-1)
#define MSF_CONF_UNSET_UINT     (uint32_t)-1
#define MSF_INT32_LEN           (sizeof("-2147483648") - 1)
#define MSF_INT64_LEN           (sizeof("-9223372036854775808") - 1)


} /**************************** end namespace BASE ****************************/
} /**************************** end namespace MSF  ****************************/
#endif
