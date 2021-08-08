#ifndef BASE_CPU_TIME_H_
#define BASE_CPU_TIME_H_

#include <stdint.h>

namespace MSF {

// Returns total CPU time of a current process in nanoseconds.
// Time base is unknown, therefore use only to calculate deltas.
int64_t GetProcessCpuTimeNanos();

// Returns total CPU time of a current thread in nanoseconds.
// Time base is unknown, therefore use only to calculate deltas.
int64_t GetThreadCpuTimeNanos();

}  // namespace MSF

#endif  // BASE_CPU_TIME_H_
