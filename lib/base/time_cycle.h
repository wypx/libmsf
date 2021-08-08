#ifndef BASE_TIMECYCLE_H_
#define BASE_TIMECYCLE_H_

#include <algorithm>
#include <cstdint>

namespace MSF {

class TimeCycle {
 public:
  TimeCycle();
  explicit TimeCycle(uint64_t tsc);

  void Swap(TimeCycle &that) {
    std::swap(tsc_since_epoch_, that.tsc_since_epoch_);
  }
  bool Valid() const { return tsc_since_epoch_ > 0; }
  uint64_t tsc_since_epoch() const { return tsc_since_epoch_; }
  uint64_t MicroSecSinceEpoch() const {
    return tsc_since_epoch_ / tsc_rate_mhz_;
  }
  uint64_t SecondSinceEpoch() const { return tsc_since_epoch_ / tsc_rate_; }
  static TimeCycle Now();
  static uint64_t GetTimeCycle();
  static uint64_t MicroSecNow();
  static uint64_t ToMicroSec(uint64_t cycle);
  static uint64_t SecondNow();

  static TimeCycle Invalid() { return TimeCycle(); }
  static uint64_t tsc_rate() { return tsc_rate_; }
  static double TimeDifferenceSecond(TimeCycle high, TimeCycle low);
  static int64_t TimeDifferenceMicroSec(TimeCycle high, TimeCycle low);
  static TimeCycle AddTime(TimeCycle time_cycle, double seconds);

  static const int kMicroSecondsPerSecond = 1000 * 1000;

 private:
  uint64_t tsc_since_epoch_;
  static uint64_t tsc_rate_;
  static uint64_t tsc_rate_mhz_;
};

inline bool operator<(TimeCycle lhs, TimeCycle rhs) {
  return lhs.tsc_since_epoch() < rhs.tsc_since_epoch();
}

inline bool operator==(TimeCycle lhs, TimeCycle rhs) {
  return lhs.tsc_since_epoch() == rhs.tsc_since_epoch();
}

}  // namespace MSF

#endif
