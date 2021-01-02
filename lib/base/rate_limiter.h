#ifndef RTC_BASE_RATE_LIMITER_H_
#define RTC_BASE_RATE_LIMITER_H_

#include <stddef.h>
#include <stdint.h>
#include <mutex>

#include "rate_statistics.h"
#include "gcc_attr.h"
#include "time_utils.h"
#include "noncopyable.h"

namespace MSF {

// Class used to limit a bitrate, making sure the average does not exceed a
// maximum as measured over a sliding window. This class is thread safe; all
// methods will acquire (the same) lock befeore executing.
class RateLimiter : public Noncopyable {
 public:
  RateLimiter(int64_t max_window_ms);
  ~RateLimiter();

  // Try to use rate to send bytes. Returns true on success and if so updates
  // current rate.
  bool TryUseRate(size_t packet_size_bytes);

  // Set the maximum bitrate, in bps, that this limiter allows to send.
  void SetMaxRate(uint32_t max_rate_bps);

  // Set the window size over which to measure the current bitrate.
  // For example, irt retransmissions, this is typically the RTT.
  // Returns true on success and false if window_size_ms is out of range.
  bool SetWindowSize(int64_t window_size_ms);

 private:
  std::mutex mutex_;
  ;
  RateStatistics current_rate_;
  int64_t window_size_ms_;
  uint32_t max_rate_bps_;
};

}  // namespace webrtc

#endif  // RTC_BASE_RATE_LIMITER_H_
