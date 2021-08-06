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
#ifndef LIB_POLLER_H_
#define LIB_POLLER_H_

#include <unistd.h>

#include <deque>
#include <iostream>
#include <map>
#include <mutex>
#include <vector>

#include "event_loop.h"

using namespace MSF;

namespace MSF {

class Poller {
 public:
  Poller(EventLoop* loop);
  virtual ~Poller();

  /// Polls the I/O events.
  /// Must be called in the loop thread.
  virtual int Poll(int timeout_ms, EventList* active_channels) = 0;

  /// Changes the interested I/O events.
  /// Must be called in the loop thread.
  virtual void UpdateEvent(Event* ev) = 0;

  /// Remove the channel, when it destructs.
  /// Must be called in the loop thread.
  virtual void RemoveEvent(Event* ev) = 0;

  virtual bool HasEvent(Event* ev) const;

  static Poller* NewDefaultPoller(EventLoop* loop);

  void AssertInLoopThread() const { owner_loop_->AssertInLoopThread(); }

  bool has_signal_profile() const {
    return owner_loop_->flags() & LOOP_BLOCK_SIGPROF;
  }

 protected:
  std::map<int, Event*> events_;

 private:
  EventLoop* owner_loop_;
};

}  // namespace MSF
#endif