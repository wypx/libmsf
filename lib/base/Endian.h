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
#ifndef BASE_ENDIAN_H
#define BASE_ENDIAN_H

#include <sys/types.h>
#include <byteswap.h>
#include <endian.h>

namespace MSF {
namespace BASE {

#if __BYTE_ORDER == __BIG_ENDIAN
#define cpu_to_le16(x)      bswap_16(x)
#define le16_to_cpu(x)      bswap_16(x)
#define cpu_to_le32(x)      bswap_32(x)
#define le32_to_cpu(x)      bswap_32(x)
#define cpu_to_le64(x)      bswap_64(x)
#define le64_to_cpu(x)      bswap_64(x)
#define cpu_to_be16(x)      (x)
#define be16_to_cpu(x)      (x)
#define cpu_to_be32(x)      (x)
#define be32_to_cpu(x)      (x)
#define cpu_to_be64(x)      (x)
#define be64_to_cpu(x)      (x)
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#define cpu_to_le16(x)      (x)
#define le16_to_cpu(x)      (x)
#define cpu_to_le32(x)      (x)
#define le32_to_cpu(x)      (x)
#define cpu_to_le64(x)      (x)
#define le64_to_cpu(x)      (x)
#define cpu_to_be16(x)      bswap_16(x)
#define be16_to_cpu(x)      bswap_16(x)
#define cpu_to_be32(x)      bswap_32(x)
#define be32_to_cpu(x)      bswap_32(x)
#define cpu_to_be64(x)      bswap_64(x)
#define be64_to_cpu(x)      bswap_64(x)
#else
#error "unknown endianess!"
#endif

} /**************************** end namespace BASE ****************************/
} /**************************** end namespace MSF  ****************************/
#endif

