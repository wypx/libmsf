
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
#ifndef __MSF_EPOLLPOLLER_H__
#define __MSF_EPOLLPOLLER_H__

#include <event/Poller.h>
#include <sys/epoll.h>

using namespace MSF::EVENT;

namespace MSF {
namespace EVENT {

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

  int poll(int timeoutMs, EventList* activeEvents) override;
  void updateEvent(Event* ev) override;
  void removeEvent(Event* ev) override;

 private:
  int _epFd;
  const uint32_t _maxEpEventNum = 1024;
  // std::vector<struct epoll_event> _epEvents;
  struct epoll_event _epEvents[1024];
  static const int _initEventListSize = 16;

  static const char* operationToString(int op);

  bool createEpSocket();
  bool addEpEvent(const Event* ev);
  bool modEpEvent(const Event* ev);
  bool delEpEvent(const Event* ev);

  void fillActiveEvents(int numEvents, EventList* activeEvents);
};

}  // namespace EVENT
}  // namespace MSF

#endif