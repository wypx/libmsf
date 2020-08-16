#include "Event.h"
#include "Poller.h"

Poller::Poller(EventLoop* loop) : _ownerLoop(loop) {}

Poller::~Poller() = default;

bool Poller::hasEvent(Event* ev) const {
  assertInLoopThread();
  EventMap::const_iterator it = _events.find(ev->fd());
  return it != _events.end() && it->second == ev;
}