#include "poller.h"

#include "event.h"

Poller::Poller(EventLoop* loop) : owner_loop_(loop) {}

Poller::~Poller() = default;

bool Poller::HasEvent(Event* ev) const {
  AssertInLoopThread();
  auto it = events_.find(ev->fd());
  return it != events_.end() && it->second == ev;
}