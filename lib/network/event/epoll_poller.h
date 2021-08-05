
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
#ifndef EVENT_EPOLLPOLLER_H_
#define EVENT_EPOLLPOLLER_H_

#include <sys/epoll.h>

#include "poller.h"

using namespace MSF;

namespace MSF {

// if (flags & MSF_EVENT_READ)
//     events |= EPOLLIN;
// if (flags & MSF_EVENT_WRITE)
//     events  |= EPOLLOUT;
// if (flags & MSF_EVENT_ERROR)
//     events |= EPOLLERR;
// if (flags & MSF_EVENT_ET)
//     events |= EPOLLET;
// if (flags & MSF_EVENT_CLOSED)
//     events |= EPOLLRDHUP;
// if (flags & MSF_EVENT_FINALIZE)
//     events |= EPOLLHUP;
// if (flags & MSF_EVENT_PERSIST)
//     events |= EPOLLONESHOT;
enum EventFlag {
  MSF_EVENT_TIMEOUT = 1 << 0,
  MSF_EVENT_READ = 1 << 1,
  MSF_EVENT_WRITE = 1 << 2,
  MSF_EVENT_SIGNAL = 1 << 3,
  MSF_EVENT_PERSIST = 1 << 4,
  MSF_EVENT_ET = 1 << 5,
  MSF_EVENT_FINALIZE = 1 << 6,
  MSF_EVENT_CLOSED = 1 << 7,
  MSF_EVENT_ERROR = 1 << 8,
  MSF_EVENT_EXCEPT = 1 << 9,
};

class EPollPoller : public Poller {
 public:
  EPollPoller(EventLoop* loop);
  ~EPollPoller() override;

  int Poll(int timeout_ms, EventList* active_events) override;
  void UpdateEvent(Event* ev) override;
  void RemoveEvent(Event* ev) override;

 private:
  int ep_fd_ = -1;
  static const uint32_t kMaxEpEventNumber = 1024;
  std::vector<struct epoll_event> ep_events_(kMaxEpEventNumber);
  static const int kInitEventListSize = 16;

  static const char* OperationToString(int op);

  bool CreateEpSocket();
  bool AddEvent(const Event* ev);
  bool ModEvent(const Event* ev);
  bool DelEvent(const Event* ev);

  void FillActiveEvents(int num_events, EventList* active_events);
};

}  // namespace MSF

#endif