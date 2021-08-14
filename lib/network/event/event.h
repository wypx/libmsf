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

#include <functional>
#include <mutex>

#include "bitops.h"
#include "buffer.h"

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

  void HandleEvent(uint64_t receiveTime);
  void HandleEventWithGuard(int receiveTime);
  void SetSuccCallback(const EventCallback& cb);
  void SetReadCallback(const EventCallback& cb);
  void SetWriteCallback(const EventCallback& cb);
  void SetCloseCallback(const EventCallback& cb);
  void SetErrorCallback(const EventCallback& cb);

  int fd() const { return fd_; }
  int events() const { return events_; }
  void set_events(uint16_t event) noexcept { events_ = event; }
  void set_revents(uint16_t event) noexcept { revents_ = event; }

  // void set_revents(int revt) noexcept { revents_ = revt; }

  // void EnableReading() {
  //   events_ |= kReadEvent;
  //   Update();
  // }
  // void DisableReading() {
  //   events_ &= ~kReadEvent;
  //   Update();
  // }
  // void EnableWriting() {
  //   events_ |= kWriteEvent;
  //   Update();
  // }
  // void DisableWriting() {
  //   events_ &= ~kWriteEvent;
  //   Update();
  // }
  // void EnableClosing() {
  //   events_ |= kCloseEvent | kErrorEvent;
  //   Update();
  // }
  // void DisableAll() {
  //   events_ = kNoneEvent;
  //   Update();
  // }
  bool IsWriting() const { return events_ & kWriteEvent; }
  bool IsReading() const { return events_ & kReadEvent; }
  bool IsNoneEvent() const { return events_ == kNoneEvent; }

  // void Remove();
  EventLoop* OwnerLoop() { return loop_; };

  // for Poller
  int index() { return index_; }
  void set_index(int idx) { index_ = idx; }

  bool is_server() const { return is_server_; }
  int bid() const { return bid_; }
  void set_bid(int bid) { bid_ = bid; }

  void set_read_bytes(size_t bytes) noexcept { read_bytes_ = bytes; }
  size_t read_bytes() const { return read_bytes_; }

  std::string ReventsToString();
  std::string EventsToString();
  std::string EventsToString(int fd, int ev);

 private:
  EventLoop* loop_;
  int bid_;
  size_t read_bytes_;
  bool is_server_ = false;
  int fd_ = -1; /* not owned right of this socket fd*/
  uint16_t events_ = kReadEvent | kCloseEvent | kErrorEvent;
  uint16_t revents_;  // it's the received event types of epoll or poll
  int index_ = kNew;  // used by Poller.
  bool added_to_loop_;
  bool event_handling_;
  EventCallback succ_cb_;
  EventCallback write_cb_;
  EventCallback read_cb_;
  EventCallback close_cb_;

  std::mutex mutex_;

  void Update();

 public:
  static const uint16_t kNoneEvent;
  static const uint16_t kReadEvent;
  static const uint16_t kWriteEvent;
  static const uint16_t kErrorEvent;
  static const uint16_t kCloseEvent;
  static const uint16_t kAcceptEvent;
};

}  // namespace MSF
#endif