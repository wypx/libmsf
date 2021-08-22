/**************************************************************************
 *
 * Copyright (c) 2017-2021, luotang.me <wypx520@gmail.com>, China.
 * All rights reserved.
 *
 * Distributed under the terms of the GNU General Public License v2.
 *
 * This software is provided 'as is' with no explicit or implied warranties
 * in respect of its properties, including, but not limited to, correctness
 * and/or fitness for purpose.
 *
 **************************************************************************/
#include "timer.h"

#include <base/logging.h>

#include <cassert>

#include "atomic.h"
#include "event_loop.h"
#include "mem.h"
#include "thread.h"
#include "time_utils.h"

using namespace MSF;

namespace MSF {

HeapTimer::~HeapTimer() {}

void HeapTimer::AddTimer(const TimerCb& cb, const double interval,
                         const enum TimerStrategy strategy) {
  // std::lock_guard<std::mutex> lock(mutex_);
  if (unlikely(queue_.size() > max_timer_)) {
    LOG(ERROR) << "Up to max event num: " << max_timer_;
    return;
  }
  queue_.push(std::move(TimerItem(cb, interval, strategy)));
  // LOG(ERROR) << "Now event num: " << queue_.size();
}

void HeapTimer::RemoveTimer(const uint32_t id) {
  loop_->RunInLoop(std::bind(&HeapTimer::CancelInLoop, this, id));
}

void HeapTimer::CancelInLoop(const uint32_t id) {
  // loop_->assertInLoopThread();
  // ActiveTimerSet::iterator it = activeTimers_.find(id);
  // if (it != activeTimers_.end()) {
  //     activeTimers_.erase(it);
  // }
}

double HeapTimer::TimerInLoop() {
  // std::lock_guard<std::mutex> lock(mutex_);
  // LOG_WARN << "queue_ size: " << queue_.size();
  while (!queue_.empty()) {
    const TimePoint currTp = CurrentMilliTimePoint();
    {
      /* Check wether min interval is expired */
      auto largest = queue_.top();
      if (!largest.IsExpired(currTp)) {
        double timePassed = GetDurationMilliSecond(largest.expired_, currTp);
        // LOG(INFO) << "Next timeout is: " << (largest.interval_ - timePassed);
        return (largest.interval_ - timePassed);
      }

      // LOG_WARN << "Timer execute ms: " << largest.expired_;

      queue_.pop();
      assert(largest.cb_);

      /* When error happed, return TIMER_ONESHOT to cancel */
      largest.cb_();
      // if (largest.cb_() == TIMER_ONESHOT) {
      //     largest.strategy_ = TIMER_LIMITED;
      //     largest.exeTimes_ = 1;
      //     LOG(INFO) << "Timer cb ret is zero";
      // }
      if (largest.strategy_ == TIMER_PERSIST ||
          (largest.strategy_ == TIMER_LIMITED && --largest.exeTimes_ > 0)) {
        largest.expired_ = CurrentMilliTimePoint();
        queue_.push(std::move(largest));
        // LOG(INFO) << "Timer add to list again, size:" << queue_.size();
      }
    }
  }
  return 0;
  ;
}

void HeapTimer::StartTimer() {
  // std::thread timer_thread([this]() {
  //     timerThread();
  // });
  // https://www.cnblogs.com/little-ant/p/3312841.html
  // timer_thread.detach();

  // std::this_thread::get_id()
  // std::thread::id timer_id = timer_thread.get_id();
}

void HeapTimer::StopTimer() {}
void HeapTimer::UpdateTime() {}

uint32_t HeapTimer::GetTimerNumber() { return queue_.size(); }

FdTimer::~FdTimer() {}

void FdTimer::AddTimer(const TimerCb& cb, const double interval,
                       const enum TimerStrategy strategy) {}
void FdTimer::RemoveTimer(uint32_t id) {}
void FdTimer::StopTimer() {}
void FdTimer::StartTimer() {
#if 0
    // 创建timer fd
    int fd = timerfd_create(CLOCK_REALTIME, 0);
    if (fd < 0) {
        // _LOG_LAST_ERROR("timerfd_create failed(%s)", strerror(errno));
        return ;
    }

    //O_NONBLOCK

    // 设置超时时间
    struct itimerspec timeout;
    timeout.it_value.tv_sec     = timeout_ms / 1000;
    timeout.it_value.tv_nsec    = (timeout_ms % 1000) * 1000 * 1000;
    timeout.it_interval.tv_sec  = timeout.it_value.tv_sec;
    timeout.it_interval.tv_nsec = timeout.it_value.tv_nsec;

    if (timerfd_settime(fd, 0, &timeout, NULL) < 0) {
        close(fd);
        // _LOG_LAST_ERROR("timerfd_settime %d failed(%s)", fd, strerror(errno));
        return kSYSTEM_ERROR;
    }
#endif
}
uint32_t FdTimer::GetTimerNumber() { return 0; }

double FdTimer::TimerInLoop() { return 0; }

void FdTimer::UpdateTime() {
  // const auto now = std::chrono::steady_clock::now();
  // const auto now = CurrentMilliTime();
  // if (xx > now )
}

}  // namespace MSF