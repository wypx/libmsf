
#include "crc.h"
#include "gcc_attr.h"

#if (defined(__amd64) || defined(__x86_64))
#include <cpuid.h>
#endif

#include <smmintrin.h>

#include <butil/logging.h>

using namespace MSF;

namespace MSF {

extern CRCFunction g_Crc32;

// 高性能CRC32
// https://blog.csdn.net/weixin_40870382/article/details/83183074?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromBaidu-2.control&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromBaidu-2.control
// https://blog.csdn.net/chihuzhong7954/article/details/100930079?utm_medium=distribute.pc_relevant.none-task-blog-searchFromBaidu-8.control&depth_1-utm_source=distribute.pc_relevant.none-task-blog-searchFromBaidu-8.control

#if defined(HAVE_CRC32_HARDWARE)
#if (defined(__amd64) || defined(__x86_64))
/*
 * checksum_hw --
 *	Return a checksum for a chunk of memory, computed in hardware
 *	using 8 byte steps.
 */
static uint32_t checksum_hw(const uint8_t *chunk, size_t len) {
  uint32_t crc;
  size_t nqwords;
  const uint8_t *p;
  const uint64_t *p64;

  crc = 0xffffffff;

  /* Checksum one byte at a time to the first 4B boundary. */
  for (p = chunk; ((uintptr_t)p & (sizeof(uint32_t) - 1)) != 0 && len > 0;
       ++p, --len) {
    __asm__ __volatile__(".byte 0xF2, 0x0F, 0x38, 0xF0, 0xF1"
                         : "=S"(crc)
                         : "0"(crc), "c"(*p));
  }

  p64 = (const uint64_t *)p;
  /* Checksum in 8B chunks. */
  for (nqwords = len / sizeof(uint64_t); nqwords; nqwords--) {
    __asm__ __volatile__(".byte 0xF2, 0x48, 0x0F, 0x38, 0xF1, 0xF1"
                         : "=S"(crc)
                         : "0"(crc), "c"(*p64));
    p64++;
  }

  /* Checksum trailing bytes one byte at a time. */
  p = (const uint8_t *)p64;
  for (len &= 0x7; len > 0; ++p, len--) {
    __asm__ __volatile__(".byte 0xF2, 0x0F, 0x38, 0xF0, 0xF1"
                         : "=S"(crc)
                         : "0"(crc), "c"(*p));
  }
  return (~crc);
}
#endif

#if defined(_M_AMD64)
/*
 * checksum_hw --
 *	Return a checksum for a chunk of memory, computed in hardware
 *	using 8 byte steps.
 */
static uint32_t checksum_hw(const uint8_t *chunk, size_t len) {
  uint32_t crc;
  size_t nqwords;
  const uint8_t *p;
  const uint64_t *p64;

  crc = 0xffffffff;

  /* Checksum one byte at a time to the first 4B boundary. */
  for (p = chunk; ((uintptr_t)p & (sizeof(uint32_t) - 1)) != 0 && len > 0;
       ++p, --len) {
    crc = _mm_crc32_u8(crc, *p);
  }

  p64 = (const uint64_t *)p;
  /* Checksum in 8B chunks. */
  for (nqwords = len / sizeof(uint64_t); nqwords; nqwords--) {
    crc = (uint32_t)_mm_crc32_u64(crc, *p64);
    p64++;
  }

  /* Checksum trailing bytes one byte at a time. */
  p = (const uint8_t *)p64;
  for (len &= 0x7; len > 0; ++p, len--) {
    crc = _mm_crc32_u8(crc, *p);
  }

  return (~crc);
}
#endif

inline
    __attribute__((always_inline)) uint32_t vpp_crc32c(uint8_t *s, size_t len) {
  uint32_t v = 0;

#if __x86_64__
  for (; len >= 8; len -= 8, s += 8) v = _mm_crc32_u64(v, *((uint64_t *)s));
#else
/* workaround weird GCC bug when using _mm_crc32_u32
   which happens with -O2 optimization */
#if !defined(__i686__)
  volatile("" :: : "memory");
#endif

  for (; len >= 4; len -= 4, s += 4) v = _mm_crc32_u32(v, *((uint32_t *)s));

  for (; len >= 2; len -= 2, s += 2) v = _mm_crc32_u16(v, *((uint16_t *)s));

  for (; len >= 1; len -= 1, s += 1) v = _mm_crc32_u8(v, *((uint16_t *)s));

#endif
  return v;
}

#endif /* HAVE_CRC32_HARDWARE */

// Intel硬件指令加速计算CRC32
// https://blog.csdn.net/lkkey80/article/details/43732819?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-2.control&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-2.control

/*
 * checksum_init --
 *	WiredTiger: detect CRC hardware and set the checksum function.
 */
// static void checksum_init(void) __attribute__((constructor(101)));

__attribute_cold__ __attribute_noinline__ void checksum_init(void) {
#if defined(HAVE_CRC32_HARDWARE)

#define CPUID_ECX_HAS_SSE42 (1 << 20)

#if (defined(__amd64) || defined(__x86_64))
  uint32_t eax, ebx, ecx, edx;
  __asm__ __volatile__("cpuid"
                       : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                       : "a"(1));

  if (ecx & CPUID_ECX_HAS_SSE42) {
    LOG(INFO) << "SSE42 support";
    g_Crc32 = checksum_hw;
  } else {
    LOG(INFO) << "SSE42 not support";
    g_Crc32 = checksum_sw;
  }

#elif defined(_M_AMD64)
  int cpuInfo[4] ::__cpuid(cpuInfo, 1);
  if (cpuInfo[2] & CPUID_ECX_HAS_SSE42) {
    LOG(INFO) << "SSE42 support";
    g_Crc32 = checksum_hw;
  } else {
    LOG(INFO) << "SSE42 not support";
    g_Crc32 = checksum_sw;
  }

#elif defined(__GNUC__)
  uint32_t eax, ebx, ecx, edx;
  ::__get_cpuid(1, &eax, &ebx, &ecx, &edx);
  if (ecx & bit_SSE4_2) {
    LOG(INFO) << "SSE42 support";
    g_Crc32 = checksum_hw;
  } else {
    LOG(INFO) << "SSE42 not support";
    g_Crc32 = checksum_sw;
  }
  g_Crc32 = checksum_sw;
#endif

#else  /* !HAVE_CRC32_HARDWARE */
  g_Crc32 = checksum_sw;
#endif /* HAVE_CRC32_HARDWARE */
}
}