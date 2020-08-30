#include "GetTime.h"

#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "CpuCycles.h"
#include "GccAttr.h"

namespace MSF {

#define USECS_PER_SEC 1000000ULL
#define UPDATE_TSC_INTERVAL 20000000000LL
#define INIT_TSC_INTERVAL 2000000000LL

static const char* gClockSource =
    "/sys/devices/system/clocksource/clocksource0/current_clocksource";
static double gUsPerCycle = 0.0;

static bool IsReliableTsc() {
  bool ret = false;
  FILE* file = fopen(gClockSource, "r");
  if (file != NULL) {
    char line[4];
    if (fgets(line, sizeof(line), file) != NULL && strcmp(line, "tsc") == 0) {
      ret = true;
    }
    fclose(file);
  }
  return ret;
}

static void InitReferenceTime(uint64_t* lastUs, uint64_t* lastCycles,
                              uint64_t us, uint64_t cycles) {
  if (*lastCycles == 0) {
    *lastCycles = cycles;
    *lastUs = us;
  } else {
    uint64_t diff = cycles - *lastCycles;
    if (diff > INIT_TSC_INTERVAL && gUsPerCycle == 0.0) {
      if (gUsPerCycle == 0.0) {
        gUsPerCycle = (us - *lastUs) / static_cast<double>(diff);
      }
    }
  }
}

static uint64_t GetTimeOfDay() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * USECS_PER_SEC + tv.tv_usec;
}

static void HandleFallback(uint64_t* us) {
  static __thread uint64_t tls_ref_last_returned_us;

  if (unlikely(tls_ref_last_returned_us > *us)) {
    *us = tls_ref_last_returned_us;
  } else {
    tls_ref_last_returned_us = *us;
  }
}

static bool gIsReliable = IsReliableTsc();

uint64_t GetCurrentTimeInUs() {
  static __thread uint64_t tls_ref_last_cycle;
  static __thread uint64_t tls_ref_last_us;

  // If tsc not relaible, use GTOD instead.
  if (unlikely(!gIsReliable)) {
    return GetTimeOfDay();
  }

  if (unlikely(tls_ref_last_us == 0 || gUsPerCycle == 0.0)) {
    uint64_t cycles = GetCpuCycles();
    uint64_t retus = GetTimeOfDay();
    InitReferenceTime(&tls_ref_last_us, &tls_ref_last_cycle, retus, cycles);
    return retus;
  }

  uint64_t diff = GetCpuCycles() - tls_ref_last_cycle;
  uint64_t retus;
  if (diff > UPDATE_TSC_INTERVAL) {
    // slow path and handle fall back
    tls_ref_last_cycle = GetCpuCycles();
    retus = GetTimeOfDay();
    HandleFallback(&retus);
    tls_ref_last_us = retus;
  } else {
    double offset = diff * gUsPerCycle;
    retus = tls_ref_last_us + static_cast<uint64_t>(offset);
    HandleFallback(&retus);
  }
  return retus;
}

}  // namespace MSF
