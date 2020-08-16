#ifndef CPU_CYCLES_H_
#define CPU_CYCLES_H_

#include <stdint.h>

namespace MSF {

#if defined(__x86_64__) || defined(__i386__)

static inline uint64_t GetCpuCycles() {
  uint32_t high, low;
  asm volatile("rdtsc" : "=a"(low), "=d"(high));
  return (static_cast<uint64_t>(high) << 32) | low;
}

#elif defined(__aarch64__)

static inline uint64_t GetCpuCycles() {
  uint64_t val;
  asm volatile("isb" : : : "memory");
  asm volatile("mrs %0, cntvct_el0" : "=r"(val));
  return val;
}

#endif

} // namespace MSF

#endif  // CPU_CYCLES_H_
