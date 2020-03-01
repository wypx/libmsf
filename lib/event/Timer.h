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
#ifndef __MSF_TIMER_H__
#define __MSF_TIMER_H__
/*----------------------------------------------------------------

获取当前时间，包含时间精度，使用系统时间还是CPU时间(asio里的deadline_timer和steady_timer的区别)
常用的API是：
    Windows: QueryPerformanceFrequency() 和 QueryPerformanceCounter()
    Linux: clock_gettime()
    OSX: gettimeofday()或者mach_absolute_time()
    当然在C++11里也可以偷懒使用chrono的high_resolution_clock
std::chrono::high_resolution_clock

timer容器的选择
    容器应该能够在很短的时间内找到MinValue
    最小堆的find-min复杂度是O(1)，所以蛮受人喜欢的
    STL里提供有堆的API，make_heap, push_heap, pop_heap, sort_heap

主循环线程还是另起线程
    另起线程需要做好线程间通信，asio和skynet有单独的timer线程

https://blog.csdn.net/libaineu2004/article/details/80539557
基于链表     O(1)     O(n)    O(n)
基于排序链表 O(n)     O(1)    O(1)
基于最小堆   O(lgn)   O(1)    O(1)
基于时间轮   O(1)     O(1)    O(1)

定时器的几种实现方式:
1. stl min heap+epoll的超时机制
    参考：本代码的实现方式
    参考：https://blog.csdn.net/w616589292/article/details/45694987
    参考：https://blog.csdn.net/liushall/article/details/81331591
    参考：https://blog.csdn.net/jasonliuvip/article/details/24738605
    参考：https://blog.csdn.net/gbjj123/article/details/25155501

    这是boost.asio的实现的timer_queue，用的是最小堆
    asio/timer_queue.hpp at master · chriskohlhoff/asio · GitHub

    这是libuv的timer，采用的是红黑树实现（windows），linux下还是最小堆
    libuv/timer.c at v1.x · libuv/libuv · GitHub

    libevent 2.0+版本用的最小堆, 之前的版本用的红黑树

2. timerfd + poll/epoll 比较有名的是muduo库
    参考：https://blog.csdn.net/qq_17308321/article/details/86622673
3. C++11 std::this_thread::sleep_for
    参考：https://blog.csdn.net/hiwubihe/article/details/84206235
4. 时间轮(Timing Wheel)算法
    参考：https://blog.csdn.net/tanjpeng/article/details/90646665
    参考：https://blog.csdn.net/tanjpeng/article/details/90300377
    参考：https://blog.csdn.net/jasonliuvip/article/details/23888851
    参考：https://github.com/libevent/timeout
5. 升序链表
    这是云风的skynet timer实现，采用链表实现
    skynet/skynet_timer.c at master · cloudwu/skynet · GitHub
6. sleep，usleep和nanosleep, RTC机制, select()
    参考：https://blog.csdn.net/hpu11/article/details/79588563

7.
6. 其他
    参考：https://blog.csdn.net/walkerkalr/article/details/36869913
----------------------------------------------------------------*/
#include <base/Time.h>

#include <iostream>
#include <queue>
#include <set>
#include <utility>
#include <atomic>

using namespace MSF::TIME;

namespace MSF {
namespace EVENT {

#define MAX_TIMER_EVENT_NUM 256

typedef std::function<void(void)> TimerCb;

enum TimerErrCode {
  TIMER_SUCCESS = 0,
  TIMER_ERR_PARAM = 1,
  TIMER_ERR_OUT_RANGE = 3,
  TIMER_ERR_NOT_EXIST = 4,
  TIMER_ERR_SYSTEM = 5
};

enum TimerStrategy {
  TIMER_ONESHOT, /* 执行一次就退出 */
  TIMER_LIMITED, /* 有限制执行多次 */
  TIMER_PERSIST, /* 一直执行不退出 */
};

struct TimerItem {
  uint32_t id_;
  uint32_t priority_;
  double interval_;
  TimePoint expired_;
  enum TimerStrategy strategy_;
  uint32_t exeTimes_;
  TimerCb cb_;

  // static uint32_t gTimerId = 0;

  const uint32_t timerId() { return id_; }

  bool operator<(struct TimerItem e) const {
    /* 超时时间越往后的优先级越低 */
    return (expired_ > e.expired_);
  }

  bool isExpired(const TimePoint t2) {
    if (GetDurationMilliSecond(expired_, t2) > interval_) {
      return true;
    }
    return false;
  }

  TimerItem(const TimerCb &cb, const double interval,
            const enum TimerStrategy strategy) {
    cb_ = std::move(cb);
    // id_ = gTimerId++;
    interval_ = interval;
    expired_ = CurrentMilliTimePoint();
    strategy_ = strategy;
  }
};

class EventLoop;
class BaseTimer {
 public:
  BaseTimer() : maxTimer_(MAX_TIMER_EVENT_NUM) {}
  explicit BaseTimer(EventLoop *loop, const uint32_t maxTimer)
      : loop_(loop), maxTimer_(maxTimer), mutex_() {}
  virtual ~BaseTimer() {}
  // virtual ~BaseTimer() = 0;
  virtual void addTimer(const TimerCb &cb, const double interval,
                        const enum TimerStrategy strategy) = 0;
  virtual void removeTimer(const uint32_t id) = 0;
  virtual void startTimer() = 0;
  virtual void stopTimer() = 0;
  virtual uint32_t getTimerNumber() = 0;
  virtual double timerInLoop() = 0;
  virtual void cancelInLoop(const uint32_t timerId) = 0;

 private:
  virtual void updateTime() = 0;

 protected:
  typedef std::pair<TimerItem, uint32_t> ActiveTimer;
  typedef std::set<ActiveTimer> ActiveTimerSet;

  ActiveTimerSet activeTimers_;

  EventLoop *loop_;
  /*in-class initialization of non-static data member is a C++11 extension*/
  std::atomic<bool> quit_ = false;
  std::priority_queue<TimerItem> queue_;
  uint32_t maxTimer_;
  std::mutex mutex_;
};

// https://blog.csdn.net/elloop/article/details/53402209
// http://www.imooc.com/wenda/detail/400903
// https://blog.csdn.net/fengbingchun/article/details/70505628
class HeapTimer : public BaseTimer {
 public:
  HeapTimer() : BaseTimer() {}
  explicit HeapTimer(EventLoop *loop, const int maxTimer = 1024)
      : BaseTimer(loop, maxTimer) {}
  ~HeapTimer();
  void addTimer(const TimerCb &cb, const double interval,
                const enum TimerStrategy strategy);
  void removeTimer(const uint32_t id);
  void stopTimer();
  void startTimer();
  uint32_t getTimerNumber();
  double timerInLoop();
  void cancelInLoop(const uint32_t id);

 private:
  void updateTime();
};

/*  基于timerfd实现的定时器\n
 *  复杂度:start O(1)，timeout O(1) 为保持timerid的一致性(非fd)，stop O(lgn) */
class FdTimer : public BaseTimer {
 public:
  FdTimer() : BaseTimer() {}
  explicit FdTimer(EventLoop *loop, const int maxTimer)
      : BaseTimer(loop, maxTimer) {}
  ~FdTimer();
  void addTimer(const TimerCb &cb, const double interval,
                const enum TimerStrategy strategy);
  void removeTimer(const uint32_t id);
  void stopTimer();
  void startTimer();
  uint32_t getTimerNumber();
  double timerInLoop();

 private:
  int epFd_;
  void updateTime();
};

}  // namespace EVENT
}  // namespace MSF
#endif