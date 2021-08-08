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
#include "event.h"

#include <base/logging.h>
#include <sys/epoll.h>

#include <sstream>

#include "event_loop.h"
#include "gcc_attr.h"

using namespace MSF;

namespace MSF {

const uint16_t Event::kNoneEvent = 0;
const uint16_t Event::kReadEvent = EPOLLIN | EPOLLPRI | EPOLLRDHUP;
const uint16_t Event::kWriteEvent = EPOLLOUT;
const uint16_t Event::kErrorEvent = EPOLLERR;
const uint16_t Event::kCloseEvent = EPOLLHUP;
const uint16_t Event::kAcceptEvent = kReadEvent;

Event::Event(EventLoop* loop, const int fd, const uint16_t events)
    : loop_(loop), fd_(fd), events_(events) {}

Event::~Event() {
  if (unlikely(event_handling_)) {
    LOG(ERROR) << "event handling: " << this;
  }
  if (unlikely(added_to_loop_)) {
    LOG(ERROR) << "event in loop: " << this;
  }
}

// void Event::Update() {
//   added_to_loop_ = true;
//   loop_->UpdateEvent(this);
// }

// void Event::Remove() {
//   if (unlikely(IsNoneEvent())) {
//     LOG(ERROR) << "none event, cannot remove: " << this;
//   }
//   added_to_loop_ = false;
//   loop_->RemoveEvent(this);
// }

void Event::HandleEvent(uint64_t receiveTime) {
  std::lock_guard<std::mutex> lock(mutex_);
  HandleEventWithGuard(receiveTime);
}

void Event::HandleEventWithGuard(int receiveTime) {
  event_handling_ = true;
  if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
    if (close_cb_) close_cb_();
  }

  if (revents_ & EPOLLERR) {
    if (close_cb_) close_cb_();
  }

  if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
    if (read_cb_) read_cb_();
  }
  if (revents_ & EPOLLOUT) {
    if (write_cb_) write_cb_();
  }
  event_handling_ = false;
}

std::string Event::ReventsToString() { return EventsToString(fd_, revents_); }

std::string Event::EventsToString() { return EventsToString(fd_, events_); }

std::string Event::EventsToString(int fd, int ev) {
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

void Event::SetSuccCallback(const EventCallback& cb) {
  succ_cb_ = std::move(cb);
}
void Event::SetReadCallback(const EventCallback& cb) {
  read_cb_ = std::move(cb);
}
void Event::SetWriteCallback(const EventCallback& cb) {
  write_cb_ = std::move(cb);
}
void Event::SetCloseCallback(const EventCallback& cb) {
  close_cb_ = std::move(cb);
}

}  // namespace MSF