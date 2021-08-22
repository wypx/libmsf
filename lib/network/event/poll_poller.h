

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
#ifndef EVENT_POLLPOLLER_H_
#define EVENT_POLLPOLLER_H_

#include <sys/poll.h>

#include "poller.h"
#include "sock_utils.h"

using namespace MSF;

namespace MSF {

class PollPoller : public Poller {
 public:
  PollPoller(EventLoop* loop);
  ~PollPoller() override;

  int Poll(int timeout_ms, EventList* active_events) override;
  void UpdateEvent(Event* ev) override;
  void RemoveEvent(Event* ev) override;

 private:
  void FillActiveEvents(int num_events, EventList* active_events);

  typedef std::vector<struct pollfd> PollFdList;
  PollFdList pollfds_;

  sigset_t sigset_;
};

}  // namespace MSF
#endif