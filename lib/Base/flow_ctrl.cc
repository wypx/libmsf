#include "flow_ctrl.h"
#include "TimeStamp.h"

bool TokenBucket::grant(uint64_t token_size) {
  uint64_t now_cycles = GetTimeCycle();
  tokens_ = std::min(
      capacity_, tokens_ + static_cast<int64_t>(
                               TimeCycleDiff(now_cycles, cycles_) * rate_));
  LOG(TRACE) << "After Token=" << tokens_ << ", cap=" << capacity_
             << ", token_size=" << token_size
             << ". now=" << TimeCycleMicroSeconds(now_cycles)
             << ", base_time=" << TimeCycleMicroSeconds(cycles_);
  cycles_ = now_cycles;
  if (tokens_ < 0) {
    return false;
  }
  tokens_ -= token_size;
  return true;
}

void TokenBucket::set_rate(uint64_t rate) {
  uint64_t now_cycles = GetTimeCycle();
  tokens_ = std::min(
      capacity_, tokens_ + static_cast<int64_t>(
                               TimeCycleDiff(now_cycles, cycles_) * rate_));
  cycles_ = now_cycles;
  rate_ = rate;
}
