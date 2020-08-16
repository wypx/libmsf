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
#ifndef MSF_EVENT_H_
#define MSF_EVENT_H_

#include <mutex>
#include <functional>

#include "Base/Bitops.h"
#include "Base/Buffer.h"

using namespace MSF;

namespace MSF {

namespace {
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
}  // namespace

typedef std::function<void()> EventCallback;

class EventLoop;
/// A selectable I/O channel.
///
/// This class doesn't own the file descriptor.
/// The file descriptor could be a socket,
/// an eventfd, a timerfd, or a signalfd
class Event {
 public:
  Event() = default;
  Event(EventLoop* loop, const int fd,
        const uint16_t events = kReadEvent | kCloseEvent | kErrorEvent);
  ~Event();

  void init(EventLoop* loop, const int fd,
            const uint16_t events = kReadEvent | kCloseEvent | kErrorEvent) {
    // assert(eventHandling_);
    // assert(addedToLoop_);
    loop_ = loop;
    fd_ = fd;
    events_ = events;
    revents_ = 0;
    index_ = kNew, eventHandling_ = false, addedToLoop_ = false;
  }

  void handleEvent(uint64_t receiveTime);
  void handleEventWithGuard(int receiveTime);
  void setSuccCallback(const EventCallback& cb);
  void setReadCallback(const EventCallback& cb);
  void setWriteCallback(const EventCallback& cb);
  void setCloseCallback(const EventCallback& cb);
  void setErrorCallback(const EventCallback& cb);

  int fd() const { return fd_; }
  int events() const { return events_; }
  void set_revents(int revt) { revents_ = revt; }

  void enableReading() {
    events_ |= kReadEvent;
    update();
  }
  void disableReading() {
    events_ &= ~kReadEvent;
    update();
  }
  void enableWriting() {
    events_ |= kWriteEvent;
    update();
  }
  void disableWriting() {
    events_ &= ~kWriteEvent;
    update();
  }
  void enableClosing() {
    events_ |= kCloseEvent | kErrorEvent;
    update();
  }
  void disableAll() {
    events_ = kNoneEvent;
    update();
  }
  bool isWriting() const { return events_ & kWriteEvent; }
  bool isReading() const { return events_ & kReadEvent; }
  bool isNoneEvent() const { return events_ == kNoneEvent; }

  void remove();
  EventLoop* ownerLoop() { return loop_; };

  // for Poller
  int index() { return index_; }
  void set_index(int idx) { index_ = idx; }

  std::string reventsToString();
  std::string eventsToString();
  std::string eventsToString(int fd, int ev);

 private:
  EventLoop* loop_;
  int fd_; /* not owned right of this socket fd*/
  uint16_t events_;
  uint16_t revents_;  // it's the received event types of epoll or poll
  int index_;         // used by Poller.
  bool addedToLoop_;
  bool eventHandling_;
  EventCallback succCb_;
  EventCallback writeCb_;
  EventCallback readCb_;
  EventCallback closeCb_;

  std::mutex mutex_;

  static const uint16_t kNoneEvent;
  static const uint16_t kReadEvent;
  static const uint16_t kWriteEvent;
  static const uint16_t kErrorEvent;
  static const uint16_t kCloseEvent;
  void update();
};


}  // namespace MSF
#endif