
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
#ifndef EVENT_IOURING_POLLER_H_
#define EVENT_IOURING_POLLER_H_

#include <sys/epoll.h>

#include "liburing.h"
#include "poller.h"

using namespace MSF;

namespace MSF {

// https://kkc.github.io/2020/08/19/io-uring/

class IOUringPoller : public Poller {
 public:
  IOUringPoller(EventLoop* loop);
  ~IOUringPoller() override;

  int Poll(int timeout_ms, EventList* active_events) override;
  void UpdateEvent(Event* ev) override;
  void RemoveEvent(Event* ev) override;

  // Returns true if arch is x86-64 and kernel supports io_uring
  static bool supported();

 private:
  int ep_fd_ = -1;
  static const uint32_t kEventNumber = 2048;
  // std::vector<struct epoll_event> _epEvents;
  struct epoll_event ep_events_[1024];

  // initialize io_uring
  struct io_uring ring_;

#define MAX_CONNECTIONS 4096
#define MAX_MESSAGE_LEN 2048
#define BUFFERS_COUNT MAX_CONNECTIONS

  char bufs_[BUFFERS_COUNT][MAX_MESSAGE_LEN] = {0};
  int group_id_ = 1337;

  static const int kInitEventListSize = 16;

  static const char* OperationToString(int op);

  bool AddEvent(const Event* ev);
  bool ModifyEvent(const Event* ev);
  bool DeleteEvent(const Event* ev);
  void AddProvideBuf(struct io_uring* ring, uint16_t bid, uint32_t gid,
                     Event* ev);
  void FillActiveEvents(int num_events, EventList* active_events);
};

}  // namespace MSF

#endif