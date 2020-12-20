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

#include "Timer.h"

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
  void loop();

  /* Quits loop.
   * This is not 100% thread safe, if you call through a raw pointer,
   * better to call through shared_ptr<EventLoop> for 100% safety.*/
  void quit();

  ///
  /// Time when poll returns, usually means data arrival.
  ///
  // uint64_t pollReturnTime() const { return pollReturnTime_; }

  int64_t iteration() const { return _iteration; }

  /* Runs callback immediately in the loop thread.
   * It wakes up the loop, and run the cb.
   * If in the same loop thread, cb is run within the function.
   * Safe to call from other threads.*/
  void runInLoop(Functor cb);

  /* Queues callback in the loop thread.
   * Runs after finish pooling.
   * Safe to call from other threads.*/
  void queueInLoop(Functor cb);

  size_t queueSize() const;

  /* Runs callback at 'time'.
   * Safe to call from other threads.*/
  uint64_t runAt(const double time, const TimerCb& cb);

  /* Runs callback after @c delay milliseconds.
   * Safe to call from other threads.*/
  uint64_t runAfter(const double delay, const TimerCb& cb);

  /* Runs callback every @c interval milliseconds.
   * Safe to call from other threads.*/
  uint64_t runEvery(const double interval, const TimerCb& cb);

  /* Cancels the timer.
   * Safe to call from other threads.*/
  void cancel(uint64_t timerId);

  // internal usage
  void wakeup();
  void updateEvent(Event* ev);
  void removeEvent(Event* ev);
  bool hasEvent(Event* ev);

  // pid_t threadId() const { return threadId_; }
  void assertInLoopThread() {
    if (!isInLoopThread()) {
      abortNotInLoopThread();
    }
  }

  // using namespace MSF;
  bool isInLoopThread() const;
  // bool callingPendingFunctors() const { return callingPendingFunctors_; }
  bool eventHandling() const { return _eventHandling; }

  static EventLoop* getEventLoopOfCurrentThread();

 private:
  void abortNotInLoopThread();
  void handleWakeup();  // waked up
  void doPendingFunctors();

  void printActiveEvents();  // DEBUG

  bool _looping; /* atomic */
  std::atomic<bool> _quit;
  bool _eventHandling;          /* atomic */
  bool _callingPendingFunctors; /* atomic */
  int64_t _iteration;
  const pid_t _threadId;

  std::unique_ptr<Poller> _poller;
  std::unique_ptr<HeapTimer> timer_;

  int wakeupFd_;
  // unlike in TimerQueue, which is an internal class,
  // we don't expose Channel to client.
  std::unique_ptr<Event> wakeupEvent_;

  EventList _activeEvents;
  Event* _currActiveEvent;

  mutable std::mutex _mutex;
  std::vector<Functor> _pendingFunctors;
};

}  // namespace MSF
#endif