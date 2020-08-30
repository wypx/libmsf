#include "TimeCycle.h"

#include "GetClock.h"

namespace MSF {

uint64_t TimeCycle::tsc_rate_ = 1000000 * MSF::get_cpu_mhz();
uint64_t TimeCycle::tsc_rate_mhz_ = MSF::get_cpu_mhz();

TimeCycle::TimeCycle() : tsc_since_epoch_(0) {}

TimeCycle::TimeCycle(uint64_t tsc) : tsc_since_epoch_(tsc) {}

TimeCycle TimeCycle::Now() { return TimeCycle(MSF::get_cycles()); }

uint64_t TimeCycle::GetTimeCycle() { return MSF::get_cycles(); }

// 返回微秒计时
uint64_t TimeCycle::MicroSecNow() {
  // MSF::get_cycles() * kMicroSecondsPerSecond / tsc_rate_ 会溢出
  return MSF::get_cycles() / tsc_rate_mhz_;
}

uint64_t TimeCycle::ToMicroSec(uint64_t cycle) { return cycle / tsc_rate_mhz_; }

uint64_t TimeCycle::SecondNow() { return MSF::get_cycles() / tsc_rate_; }

double TimeCycle::TimeDifferenceSecond(TimeCycle high, TimeCycle low) {
  int64_t diff = high.tsc_since_epoch() - low.tsc_since_epoch();
  return static_cast<double>(diff) / tsc_rate_;
}

int64_t TimeCycle::TimeDifferenceMicroSec(TimeCycle high, TimeCycle low) {
  int64_t diff = high.tsc_since_epoch() - low.tsc_since_epoch();
  return diff / tsc_rate_mhz_;
}

TimeCycle TimeCycle::AddTime(TimeCycle time_cycle, double seconds) {
  uint64_t delta = seconds * tsc_rate_;
  return TimeCycle(time_cycle.tsc_since_epoch() + delta);
}

}  // namespace MSF
