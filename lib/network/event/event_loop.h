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
#ifndef EVENT_EVENTLOOP_H
#define EVENT_EVENTLOOP_H

#include <unistd.h>

#include <atomic>
#include <deque>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>

#include "timer.h"
#include "mem_pool.h"

using namespace MSF;

namespace MSF {

class Event;
class Poller;

typedef std::vector<Event*> EventList;

typedef uint64_t TimerId;

/* Reactor, at most one per thread. */
class EventLoop {
 public:
  typedef std::function<void()> Functor;

  EventLoop();
  /* force out-line dtor, for std::unique_ptr members. */
  ~EventLoop();

  /* Loops forever */
  void EnterLoop();

  /* Quits loop.
   * This is not 100% thread safe, if you call through a raw pointer,
   * better to call through shared_ptr<EventLoop> for 100% safety.*/
  void QuitLoop();

  ///
  /// Time when poll returns, usually means data arrival.
  ///
  // uint64_t pollReturnTime() const { return pollReturnTime_; }

  int64_t iteration() const { return iteration_; }

  /* Runs callback immediately in the loop thread.
   * It wakes up the loop, and run the cb.
   * If in the same loop thread, cb is run within the function.
   * Safe to call from other threads.*/
  void RunInLoop(Functor cb);

  /* Queues callback in the loop thread.
   * Runs after finish pooling.
   * Safe to call from other threads.*/
  void QueueInLoop(Functor cb);

  size_t QueueSize() const;

  /* Runs callback at 'time'.
   * Safe to call from other threads.*/
  uint64_t RunAt(const double time, const TimerCb& cb);

  /* Runs callback after @c delay milliseconds.
   * Safe to call from other threads.*/
  uint64_t RunAfter(const double delay, const TimerCb& cb);

  /* Runs callback every @c interval milliseconds.
   * Safe to call from other threads.*/
  uint64_t RunEvery(const double interval, const TimerCb& cb);

  /* Cancels the timer.
   * Safe to call from other threads.*/
  void Cancel(uint64_t timerId);

  // internal usage
  void Wakeup();
  void UpdateEvent(Event* ev);
  void RemoveEvent(Event* ev);
  bool HasEvent(Event* ev);

  // pid_t threadId() const { return threadId_; }
  void AssertInLoopThread() {
    if (!IsInLoopThread()) {
      AbortNotInLoopThread();
    }
  }

  // using namespace MSF;
  bool IsInLoopThread() const;
  // bool callingPendingFunctors() const { return callingPendingFunctors_; }
  bool EventHandling() const { return event_handling_; }

  void* Malloc(size_t len);
  void Free(void* ptr);

  static EventLoop* GetEventLoopOfCurrentThread();

 private:
  void AbortNotInLoopThread();
  void HandleWakeup();  // waked up
  void DoPendingFunctors();

  void PrintActiveEvents();  // DEBUG

  bool looping_ = false; /* atomic */
  bool quit_ = false;
  bool event_handling_ = false;           /* atomic */
  bool calling_pending_functors_ = false; /* atomic */
  int64_t iteration_ = 0;
  const pid_t thread_id_;

  std::unique_ptr<Poller> poller_;
  std::unique_ptr<HeapTimer> timer_;
  std::unique_ptr<MemPool> mem_pool_;

  int wakeup_fd_ = -1;
  // unlike in TimerQueue, which is an internal class,
  // we don't expose Channel to client.
  std::unique_ptr<Event> wakeup_event_;

  EventList active_events_;
  Event* curr_active_event_;

  mutable std::mutex _mutex;
  std::vector<Functor> pending_functors_;
};

}  // namespace MSF
#endif