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

#include "EventLoop.h"

using namespace MSF;

namespace MSF {

class Poller {
 public:
  Poller(EventLoop* loop);
  virtual ~Poller();

  /// Polls the I/O events.
  /// Must be called in the loop thread.
  virtual int poll(int timeoutMs, EventList* activeChannels) = 0;

  /// Changes the interested I/O events.
  /// Must be called in the loop thread.
  virtual void updateEvent(Event* ev) = 0;

  /// Remove the channel, when it destructs.
  /// Must be called in the loop thread.
  virtual void removeEvent(Event* ev) = 0;

  virtual bool hasEvent(Event* ev) const;

  static Poller* newDefaultPoller(EventLoop* loop);

  void assertInLoopThread() const { _ownerLoop->assertInLoopThread(); }

 protected:
  typedef std::map<int, Event*> EventMap;
  EventMap _events;

 private:
  EventLoop* _ownerLoop;
};

}  // namespace MSF
#endif