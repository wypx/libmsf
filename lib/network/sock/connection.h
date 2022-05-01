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
#ifndef SOCK_CONNECTION_H_
#define SOCK_CONNECTION_H_

#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "buffer.h"
#include "callback.h"
#include "event.h"
#include "inet_address.h"

using namespace MSF;

namespace MSF {

class EventLoop;

enum ShutdownMode { kShutdownBoth, kShutdownRead, kShutdownWrite };

enum BufferType { kBufferCircle, kBufferVector };

typedef std::vector<struct iovec> BufferIovec;

class Connection : public std::enable_shared_from_this<Connection> {
 public:
  enum State {
    kStateNone,            // init connection state
    kStateConnecting,      // connection sync connecting
    kStateConnected,       // connection connected
    kStateCloseWaitWrite,  // passive or active close, but have data to send
    kStatePassiveClose,    // connection should close
    kStateActiveClose,     // connection should close
    kStateError,           // waiting to handle error
    kStateClosed,          // final state being to destroy
  };

  Connection(EventLoop *loop, int fd, bool thread_safe = true);
  virtual ~Connection();

  // send data
  void SubmitWriteBufferSafe(void *buffer, size_t size) noexcept;
  void SubmitWriteIovecSafe(BufferIovec &queue) noexcept;

  // recv in buffer
  virtual size_t ReadableLength() = 0;
  virtual size_t ReceiveData(void *data, size_t len) = 0;
  virtual size_t DrainData(size_t len) = 0;
  virtual size_t RemoveData(void *data, size_t len) = 0;

  virtual bool IsClosed() = 0;
  virtual void CloseConn() = 0;

  virtual void *Malloc(size_t len, size_t align) = 0;
  virtual void Free(void *ptr) = 0;

  int fd() noexcept { return fd_; }
  void set_read_water_mark(int64_t mark) noexcept { read_water_mark_ = mark; }
  int64_t read_water_mark() const noexcept { return read_water_mark_; }
  inline void set_active_ts(uint64_t ts) noexcept { active_ts_ = ts; }
  inline uint64_t active_ts() const noexcept { return active_ts_; }

  inline const InetAddress &peer_addr() const noexcept { return peer_addr_; }
  inline EventLoop *loop() const noexcept { return loop_; }

  void SetConnHighWaterCb(const HighWaterCallback &cb) noexcept {
    io_water_cb_ = std::move(cb);
  }
  void SetConnWriteIOCPCb(const WriteIOCPCallback &cb) noexcept {
    write_iocp_cb_ = std::move(cb);
  }
  void SetConnSuccCb(const SuccCallback &cb) noexcept {
    event_->SetSuccCallback([this, cb] {
      HandleSuccEvent();
      cb(shared_from_this());
    });
  }
  void SetConnReadCb(const ReadCallback &cb) noexcept {
    event_->SetReadCallback([this, cb] {
      HandleReadEvent();
      cb(shared_from_this());
    });
  }
  void SetConnWriteCb(const WriteCallback &cb) noexcept {
    event_->SetWriteCallback([this, cb] {
      HandleWriteEvent();
      cb(shared_from_this());
    });
  }
  void SetConnCloseCb(const CloseCallback &cb) noexcept {
    event_->SetCloseCallback([this, cb] {
      HandleCloseEvent();
      cb(shared_from_this());
    });
  }

  void set_buffer_type(BufferType type) noexcept { buffer_type_ = type; }
  BufferType buffer_type() const noexcept { return buffer_type_; }

  void set_cid(uint64_t cid) noexcept { cid_ = cid; }
  uint64_t cid() noexcept { return cid_; }

  void set_state(State state) noexcept { state_ = state; }
  State state() noexcept { return state_; }

  void AddGeneralEvent();
  void AddWriteEvent();
  void RemoveWriteEvent();
  void RemoveAllEvent();

 protected:
  virtual void HandleSuccEvent() = 0;
  virtual void HandleReadEvent() = 0;
  virtual void HandleWriteEvent() = 0;
  virtual void HandleCloseEvent() = 0;

  void SubmitWriteIovec(BufferIovec &queue) noexcept;
  bool UpdateWriteBusyIovecSafe() noexcept;
  bool UpdateWriteBusyIovec() noexcept;
  void UpdateWriteBusyOffset(uint64_t bytes_send) noexcept;
  void ClearWritePendingIovec() noexcept;
  void ClearWriteBusyIovec() noexcept;
  virtual void Shutdown(ShutdownMode mode) = 0;

 protected:
  std::unique_ptr<Event> event_;
  EventLoop *loop_;
  uint64_t cid_;
  int fd_ = -1;
  InetAddress peer_addr_;
  State state_ = State::kStateNone;
  uint64_t active_ts_;

  BufferType buffer_type_;

  int64_t read_water_mark_;
  Buffer read_buf_;
  Buffer write_buf_;
  // not copy to circular buffer
  // mutex means whether thread sate
  std::mutex *mutex_;
  BufferIovec write_pending_queue_;
  BufferIovec write_busy_queue_;

  HighWaterCallback io_water_cb_;
  WriteIOCPCallback write_iocp_cb_;
};
}  // namespace MSF

#endif