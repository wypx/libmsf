#ifndef FLOW_CTRL_H_
#define FLOW_CTRL_H_

#include <assert.h>
#include "GccAttr.h"
#include "TimeStamp.h"
#include "CpuCycles.h"
#include "TimeCycle.h"
#include <butil/logging.h>

using namespace MSF;

double TimeStampDiff(TimeStamp a, TimeStamp b) {
  return timeDifference(a, b);
}

uint64_t GetTimeCycle() { return MSF::TimeCycle::GetTimeCycle(); }

double TimeCycleDiff(uint64_t a, uint64_t b) {
  int64_t diff = a - b;
  return static_cast<double>(diff) / MSF::TimeCycle::tsc_rate();
}

uint64_t TimeCycleSeconds(uint64_t cycles) {
  return cycles / MSF::TimeCycle::tsc_rate();
}

uint64_t TimeCycleMicroSeconds(uint64_t cycles) {
  return cycles / (MSF::TimeCycle::tsc_rate() / 1000000);
}

class TokenBucket {
 public:
  TokenBucket(uint64_t capacity, uint64_t rate)
      : tokens_(capacity),
        cycles_(GetTimeCycle()),
        capacity_(capacity),
        rate_(rate) {}

  bool grant(uint64_t token_size);

  void set_rate(uint64_t rate);

 private:
  int64_t tokens_;
  uint64_t cycles_;
  int64_t capacity_;
  int64_t rate_;
};

template <typename T = uint64_t, int size = 2>
class CompoundTokenBucket {
 public:
  CompoundTokenBucket(T* capacity, T* rate) : timestamp_() {
    for (int i = 0; i < size; ++i) {
      tokens_[i] = capacity[i];
      capacity_[i] = capacity[i];
      rate_[i] = rate[i];
      percent_[i] = 100;
    }
  }

  CompoundTokenBucket(T capacity, T rate) {
    assert(size == 1);
    tokens_[0] = capacity;
    capacity_[0] = capacity;
    rate_[0] = rate;
    percent_[0] = 100;
  }

  void reset(T* capacity, T* rate) {
    for (int i = 0; i < size; ++i) {
      capacity_[i] = capacity[i];
      rate_[i] = rate[i];
    }
  }

  void add() {
    TimeStamp now_time;
    for (int i = 0; i < size; ++i) {
      tokens_[i] = std::min(
          capacity_[i],
          tokens_[i] +
              static_cast<uint32_t>(
                  TimeStampDiff(now_time, timestamp_) * rate_[i] + 0.5) *
                  percent_[i] / 100);  // 得到最接近的整数
    }
    timestamp_ = now_time;
  }

  bool grant(T* token_size) {
    int i;

    for (i = 0; i < size; ++i) {
      // 防止大IO被永久阻塞，出现概率极小
      if (unlikely(token_size[i] > capacity_[i])) {
        LOG(INFO) << "token is larger than capacity, token=" << token_size[i]
                  << ", capacity=" << capacity_[i];
        token_size[i] = capacity_[i];
      }

      if (tokens_[i] < token_size[i]) {
        return false;
      }
      LOG(INFO) << "tokens=" << tokens_[i]
                 << ", cap=" << capacity_[i] * percent_[i] / 100
                 << ", req_token=" << token_size[i];
    }

    for (i = 0; i < size; ++i) {
      tokens_[i] -= token_size[i];
    }
    return true;
  }

  int percent(int index) const {
    if (index >= size) {
      return 0;
    }
    return percent_[index];
  }

  void set_percent(int percent, int index) {
    if (index >= size) {
      return;
    }
    if (percent < 0) {
      percent = 0;
    }
    if (percent > 100) {
      percent = 100;
    }
    percent_[index] = percent;
  }

 private:
  T tokens_[size];
  TimeStamp timestamp_;
  T capacity_[size];
  T rate_[size];
  int percent_[size];
};

class LeakyBucket {
 public:
  LeakyBucket(uint64_t rate, double precision = 0.001)
      : cycles_(GetTimeCycle()),
        rate_(rate),
        debt_(0),
        precision_(precision),
        quota_(rate * precision) {}

  bool grant(uint64_t size) {
    uint64_t now_cycles = GetTimeCycle();
    uint64_t pay_off =
        static_cast<uint64_t>(TimeCycleDiff(now_cycles, cycles_) * rate_);
    uint64_t left_debt = debt_ > pay_off ? debt_ - pay_off : 0;
    if (left_debt > quota_) {
      return false;
    }
    // debt_赋值放在grant通过之后，防止频繁grant失败因精度问题导致token计算不准
    cycles_ = now_cycles;
    debt_ = left_debt + size;
    return true;
  }

  void set_rate(uint64_t rate) {
    uint64_t now_cycles = GetTimeCycle();
    uint64_t pay_off =
        static_cast<uint64_t>(TimeCycleDiff(now_cycles, cycles_) * rate_);
    debt_ = debt_ > pay_off ? debt_ - pay_off : 0;
    cycles_ = now_cycles;
    rate_ = rate;
    quota_ = rate * precision_;
  }

 private:
  uint64_t cycles_;
  uint64_t rate_;
  uint64_t debt_;
  double precision_;
  uint64_t quota_;
};

#endif  // FLOW_CTRL_H_
