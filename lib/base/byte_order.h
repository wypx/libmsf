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
#ifndef BASE_ENDIAN_H_
#define BASE_ENDIAN_H_

#include <byteswap.h>
#include <endian.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdint.h>

namespace MSF {

#if __BYTE_ORDER == __BIG_ENDIAN
#define cpu_to_le16(x) bswap_16(x)
#define le16_to_cpu(x) bswap_16(x)
#define cpu_to_le32(x) bswap_32(x)
#define le32_to_cpu(x) bswap_32(x)
#define cpu_to_le64(x) bswap_64(x)
#define le64_to_cpu(x) bswap_64(x)
#define cpu_to_be16(x) (x)
#define be16_to_cpu(x) (x)
#define cpu_to_be32(x) (x)
#define be32_to_cpu(x) (x)
#define cpu_to_be64(x) (x)
#define be64_to_cpu(x) (x)
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#define cpu_to_le16(x) (x)
#define le16_to_cpu(x) (x)
#define cpu_to_le32(x) (x)
#define le32_to_cpu(x) (x)
#define cpu_to_le64(x) (x)
#define le64_to_cpu(x) (x)
#define cpu_to_be16(x) bswap_16(x)
#define be16_to_cpu(x) bswap_16(x)
#define cpu_to_be32(x) bswap_32(x)
#define be32_to_cpu(x) bswap_32(x)
#define cpu_to_be64(x) bswap_64(x)
#define be64_to_cpu(x) bswap_64(x)
#else
#error "unknown endianess!"
#endif

#if defined(__apple__)
#include <libkern/OSByteOrder.h>

#define htobe16(v) OSSwapHostToBigInt16(v)
#define htobe32(v) OSSwapHostToBigInt32(v)
#define htobe64(v) OSSwapHostToBigInt64(v)
#define be16toh(v) OSSwapBigToHostInt16(v)
#define be32toh(v) OSSwapBigToHostInt32(v)
#define be64toh(v) OSSwapBigToHostInt64(v)

#define htole16(v) OSSwapHostToLittleInt16(v)
#define htole32(v) OSSwapHostToLittleInt32(v)
#define htole64(v) OSSwapHostToLittleInt64(v)
#define le16toh(v) OSSwapLittleToHostInt16(v)
#define le32toh(v) OSSwapLittleToHostInt32(v)
#define le64toh(v) OSSwapLittleToHostInt64(v)

#elif defined(WIN32) || defined(WIN64) || defined(__native_client__)

#if defined(WIN32) || defined(WIN64)
#include <stdlib.h>
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif  // defined(WEBRTC_WIN)

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define htobe16(v) htons(v)
#define htobe32(v) htonl(v)
#define be16toh(v) ntohs(v)
#define be32toh(v) ntohl(v)
#define htole16(v) (v)
#define htole32(v) (v)
#define htole64(v) (v)
#define le16toh(v) (v)
#define le32toh(v) (v)
#define le64toh(v) (v)
#if defined(WEBRTC_WIN)
#define htobe64(v) _byteswap_uint64(v)
#define be64toh(v) _byteswap_uint64(v)
#endif  // defined(WEBRTC_WIN)
#if defined(__native_client__)
#define htobe64(v) __builtin_bswap64(v)
#define be64toh(v) __builtin_bswap64(v)
#endif  // defined(__native_client__)

#elif defined(WEBRTC_ARCH_BIG_ENDIAN)
#define htobe16(v) (v)
#define htobe32(v) (v)
#define be16toh(v) (v)
#define be32toh(v) (v)
#define htole16(v) __builtin_bswap16(v)
#define htole32(v) __builtin_bswap32(v)
#define htole64(v) __builtin_bswap64(v)
#define le16toh(v) __builtin_bswap16(v)
#define le32toh(v) __builtin_bswap32(v)
#define le64toh(v) __builtin_bswap64(v)
#if defined(WEBRTC_WIN)
#define htobe64(v) (v)
#define be64toh(v) (v)
#endif  // defined(WEBRTC_WIN)
#if defined(__native_client__)
#define htobe64(v) (v)
#define be64toh(v) (v)
#endif  // defined(__native_client__)
#else
#error WEBRTC_ARCH_BIG_ENDIAN or WEBRTC_ARCH_LITTLE_ENDIAN must be defined.
#endif  // __BYTE_ORDER == __LITTLE_ENDIAN

#elif defined(__linux__)
// #include <endian.h>
#else
#error "Missing byte order functions for this arch."
#endif

// Reading and writing of little and big-endian numbers from memory

inline void Set8(void* memory, size_t offset, uint8_t v) {
  static_cast<uint8_t*>(memory)[offset] = v;
}

inline uint8_t Get8(const void* memory, size_t offset) {
  return static_cast<const uint8_t*>(memory)[offset];
}

inline void SetBE16(void* memory, uint16_t v) {
  *static_cast<uint16_t*>(memory) = htobe16(v);
}

inline void SetBE32(void* memory, uint32_t v) {
  *static_cast<uint32_t*>(memory) = htobe32(v);
}

inline void SetBE64(void* memory, uint64_t v) {
  *static_cast<uint64_t*>(memory) = htobe64(v);
}

inline uint16_t GetBE16(const void* memory) {
  return be16toh(*static_cast<const uint16_t*>(memory));
}

inline uint32_t GetBE32(const void* memory) {
  return be32toh(*static_cast<const uint32_t*>(memory));
}

inline uint64_t GetBE64(const void* memory) {
  return be64toh(*static_cast<const uint64_t*>(memory));
}

inline void SetLE16(void* memory, uint16_t v) {
  *static_cast<uint16_t*>(memory) = htole16(v);
}

inline void SetLE32(void* memory, uint32_t v) {
  *static_cast<uint32_t*>(memory) = htole32(v);
}

inline void SetLE64(void* memory, uint64_t v) {
  *static_cast<uint64_t*>(memory) = htole64(v);
}

inline uint16_t GetLE16(const void* memory) {
  return le16toh(*static_cast<const uint16_t*>(memory));
}

inline uint32_t GetLE32(const void* memory) {
  return le32toh(*static_cast<const uint32_t*>(memory));
}

inline uint64_t GetLE64(const void* memory) {
  return le64toh(*static_cast<const uint64_t*>(memory));
}

// Check if the current host is big endian.
inline bool IsHostBigEndian() {
#if __BYTE_ORDER == __BIG_ENDIAN
  return true;
#else
  return false;
#endif
}

inline uint16_t HostToNetwork16(uint16_t n) { return htobe16(n); }

inline uint32_t HostToNetwork32(uint32_t n) { return htobe32(n); }

inline uint64_t HostToNetwork64(uint64_t n) { return htobe64(n); }

inline uint16_t NetworkToHost16(uint16_t n) { return be16toh(n); }

inline uint32_t NetworkToHost32(uint32_t n) { return be32toh(n); }

inline uint64_t NetworkToHost64(uint64_t n) { return be64toh(n); }

}  // namespace MSF
#endif