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
#include "Event.h"

#include <butil/logging.h>
#include <sys/epoll.h>

#include <sstream>

#include "EventLoop.h"
#include "GccAttr.h"

using namespace MSF;

namespace MSF {

const uint16_t Event::kNoneEvent = 0;
const uint16_t Event::kReadEvent = EPOLLIN | EPOLLPRI | EPOLLRDHUP;
const uint16_t Event::kWriteEvent = EPOLLOUT;
const uint16_t Event::kErrorEvent = EPOLLERR;
const uint16_t Event::kCloseEvent = EPOLLHUP;

Event::Event(EventLoop* loop, const int fd, const uint16_t events) {
  init(loop, fd, events);
}

Event::~Event() {
  if (unlikely(eventHandling_)) {
    LOG(ERROR) << "Event handling: " << this;
  }
  if (unlikely(addedToLoop_)) {
    LOG(ERROR) << "Event in loop: " << this;
  }
  if (loop_->isInLoopThread()) {
    if (unlikely(loop_->hasEvent(this))) {
      LOG(ERROR) << "Loop has event: " << loop_;
    }
  }
}

void Event::update() {
  addedToLoop_ = true;
  loop_->updateEvent(this);
}

void Event::remove() {
  if (unlikely(isNoneEvent())) {
    LOG(ERROR) << "None event, cannot remove: " << this;
  }
  addedToLoop_ = false;
  loop_->removeEvent(this);
}

void Event::handleEvent(uint64_t receiveTime) {
  std::lock_guard<std::mutex> lock(mutex_);
  handleEventWithGuard(receiveTime);
}

void Event::handleEventWithGuard(int receiveTime) {
  eventHandling_ = true;
  if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
    if (closeCb_) closeCb_();
  }

  if (revents_ & EPOLLERR) {
    if (closeCb_) closeCb_();
  }

  if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
    if (readCb_) readCb_();
  }
  if (revents_ & EPOLLOUT) {
    if (writeCb_) writeCb_();
  }
  eventHandling_ = false;
}

std::string Event::reventsToString() { return eventsToString(fd_, revents_); }

std::string Event::eventsToString() { return eventsToString(fd_, events_); }

std::string Event::eventsToString(int fd, int ev) {
  std::stringstream oss;
  oss << fd << ": ";
  if (ev & EPOLLIN) oss << "EPOLLIN ";
  if (ev & EPOLLPRI) oss << "EPOLLPRI ";
  if (ev & EPOLLOUT) oss << "EPOLLOUT ";
  if (ev & EPOLLHUP) oss << "EPOLLHUP ";
  if (ev & EPOLLRDHUP) oss << "EPOLLRDHUP ";
  if (ev & EPOLLERR) oss << "EPOLLERR ";
  // if (ev & POLLNVAL)
  //   oss << "NVAL ";

  return oss.str();
}

void Event::setSuccCallback(const EventCallback& cb) {
  succCb_ = std::move(cb);
}
void Event::setReadCallback(const EventCallback& cb) {
  readCb_ = std::move(cb);
}
void Event::setWriteCallback(const EventCallback& cb) {
  writeCb_ = std::move(cb);
}
void Event::setCloseCallback(const EventCallback& cb) {
  closeCb_ = std::move(cb);
}

}  // namespace MSF