#include "flow_ctrl.h"

#include "time_stamp.h"

bool TokenBucket::grant(uint64_t token_size) {
  uint64_t now_cycles = GetTimeCycle();
  tokens_ = std::min(capacity_,
                     tokens_ + static_cast<int64_t>(
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

// 实际的流控不准,比如前期无法预测数据大小,
// 只有等数据真正读取上来之后才知道具体大小.
// - 4M + 3M (实际有效数据量只有1M)
void TokenBucket::add(uint64_t refound_size) {
  uint64_t now_cycles = GetTimeCycle();
  tokens_ = std::min(
      capacity_,
      tokens_ + int64_t(refound_size) +
          static_cast<int64_t>(TimeCycleDiff(now_cycles, cycles_) * rate_));
  LOG(TRACE) << "After Token=" << tokens_ << ", cap=" << capacity_
             << ". now=" << TimeCycleMicroSeconds(now_cycles)
             << ", base_time=" << TimeCycleMicroSeconds(cycles_);
  cycles_ = now_cycles;
}

void TokenBucket::set_rate(uint64_t rate) {
  uint64_t now_cycles = GetTimeCycle();
  tokens_ = std::min(capacity_,
                     tokens_ + static_cast<int64_t>(
                                   TimeCycleDiff(now_cycles, cycles_) * rate_));
  cycles_ = now_cycles;
  rate_ = rate;
}
