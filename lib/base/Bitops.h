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
#ifndef __MSF_BIT_H__
#define __MSF_BIT_H__

#include <sys/types.h>
#include <byteswap.h>
#include <endian.h>

namespace MSF {
namespace BASE {

#ifndef BIT_SET
#define BIT_SET(v,f)        (v) |= (f)
#endif

#ifndef BIT_CLEAR
#define BIT_CLEAR(v,f)      (v) &= ~(f)
#endif

#ifndef BIT_IS_SET
#define BIT_IS_SET(v,f)     ((v) & (f))
#endif

#define MSF_TEST_BITS(mask, addr)   (((*addr) & (mask)) != 0)
#define MSF_CLR_BITS(mask, addr)    ((*addr) &= ~(mask))
#define MSF_SET_BITS(mask, addr)    ((*addr) |= (mask))

#define MSF_TEST_BIT(nr, addr)      (*(addr) & (1ULL << (nr)))
#define MSF_CLR_BIT(nr, addr)       (*(addr) &=  ~(1ULL << (nr)))
#define MSF_SET_BIT(nr, addr)       (*(addr) |=  (1ULL << (nr)))


#if __BYTE_ORDER == __BIG_ENDIAN
#define cpu_to_le16(x)      bswap_16(x)
#define le16_to_cpu(x)      bswap_16(x)
#define cpu_to_le32(x)      bswap_32(x)
#define le32_to_cpu(x)      bswap_32(x)
#define cpu_to_be16(x)      (x)
#define be16_to_cpu(x)      (x)
#define cpu_to_be32(x)      (x)
#define be32_to_cpu(x)      (x)
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#define cpu_to_le16(x)      (x)
#define le16_to_cpu(x)      (x)
#define cpu_to_le32(x)      (x)
#define le32_to_cpu(x)      (x)
#define cpu_to_be16(x)      bswap_16(x)
#define be16_to_cpu(x)      bswap_16(x)
#define cpu_to_be32(x)      bswap_32(x)
#define be32_to_cpu(x)      bswap_32(x)
#else
#error "unknown endianess!"
#endif

} /**************************** end namespace BASE ****************************/
} /**************************** end namespace MSF  ****************************/
#endif

