
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
#include "poll_poller.h"

#include <base/logging.h>

#include <algorithm>

#include "event.h"
#include "gcc_attr.h"
#include "utils.h"

namespace MSF {

PollPoller::PollPoller(EventLoop* loop) : Poller(loop) {}

PollPoller::~PollPoller() = default;

int PollPoller::Poll(int timeout_ms, EventList* active_events) {
  // XXX pollfds_ shouldn't change
  if (::pthread_sigmask(SIG_BLOCK, &sigset_, nullptr)) {
  }
  int num_events = ::poll(&*pollfds_.begin(), pollfds_.size(), timeout_ms);
  if (::pthread_sigmask(SIG_UNBLOCK, &sigset_, nullptr)) {
  }
  int saved_errno = errno;
  if (num_events > 0) {
    LOG(TRACE) << num_events << " events happened";
    FillActiveEvents(num_events, active_events);
  } else if (num_events == 0) {
    LOG(TRACE) << " nothing happened";
  } else {
    if (saved_errno != EINTR) {
      errno = saved_errno;
    }
  }
  return 0;
}

void PollPoller::FillActiveEvents(int num_events, EventList* active_channels) {
  for (auto pfd = pollfds_.begin(); pfd != pollfds_.end() && num_events > 0;
       ++pfd) {
    if (pfd->revents > 0) {
      --num_events;
      auto it = events_.find(pfd->fd);
      assert(it != events_.end());
      Event* ev = it->second;
      assert(ev->fd() == pfd->fd);
      ev->set_revents(pfd->revents);
      // pfd->revents = 0;
      active_channels->push_back(ev);
    }
  }
}

void PollPoller::UpdateEvent(Event* ev) {
  Poller::AssertInLoopThread();
  LOG(TRACE) << "fd = " << ev->fd() << " events = " << ev->events();
  if (ev->index() < 0) {
    // a new one, add to pollfds_
    assert(events_.find(ev->fd()) == events_.end());
    struct pollfd pfd;
    pfd.fd = ev->fd();
    pfd.events = static_cast<short>(ev->events());
    pfd.revents = 0;
    pollfds_.push_back(pfd);
    int idx = static_cast<int>(pollfds_.size()) - 1;
    ev->set_index(idx);
    events_[pfd.fd] = ev;
  } else {
    // update existing one
    assert(events_.find(ev->fd()) != events_.end());
    assert(events_[ev->fd()] == ev);
    int idx = ev->index();
    assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
    struct pollfd& pfd = pollfds_[idx];
    assert(pfd.fd == ev->fd() || pfd.fd == -ev->fd() - 1);
    pfd.fd = ev->fd();
    pfd.events = static_cast<short>(ev->events());
    pfd.revents = 0;
    if (ev->IsNoneEvent()) {
      // ignore this pollfd
      pfd.fd = -ev->fd() - 1;
    }
  }
}

void PollPoller::RemoveEvent(Event* ev) {
  Poller::AssertInLoopThread();
  LOG(TRACE) << "fd = " << ev->fd();
  assert(events_.find(ev->fd()) != events_.end());
  assert(events_[ev->fd()] == ev);
  assert(ev->IsNoneEvent());
  int idx = ev->index();
  assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
  const struct pollfd& pfd = pollfds_[idx];
  (void)pfd;
  assert(pfd.fd == -ev->fd() - 1 && pfd.events == ev->events());
  size_t n = events_.erase(ev->fd());
  assert(n == 1);
  (void)n;
  if (implicit_cast<size_t>(idx) == pollfds_.size() - 1) {
    pollfds_.pop_back();
  } else {
    int channelAtEnd = pollfds_.back().fd;
    iter_swap(pollfds_.begin() + idx, pollfds_.end() - 1);
    if (channelAtEnd < 0) {
      channelAtEnd = -channelAtEnd - 1;
    }
    events_[channelAtEnd]->set_index(idx);
    pollfds_.pop_back();
  }
}
}  // namespace MSF