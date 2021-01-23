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

#include "callback.h"
#include "buffer.h"

using namespace MSF;

namespace MSF {

class EventLoop;

enum ShutdownMode {
  kShutdownBoth,
  kShutdownRead,
  kShutdownWrite
};

enum BufferType {
  kBufferCircle,
  kBufferVector
};

typedef std::vector<struct iovec> BufferIovec;

class Connection : public std::enable_shared_from_this<Connection> {
 public:
  enum State {
    kStateNone,            // init connection state
    kStateConnected,       // connection connected
    kStateCloseWaitWrite,  // passive or active close, but have data to send
    kStatePassiveClose,    // connection should close
    kStateActiveClose,     // connection should close
    kStateError,           // waiting to handle error
    kStateClosed,          // final state being to destroy
  };

  Connection(bool thread_safe = true);
  virtual ~Connection();

  virtual bool HandleReadEvent() = 0;
  virtual bool HandleWriteEvent() = 0;
  virtual void HandleErrorEvent() = 0;

  void SetConnHighWaterCb(const HighWaterCallback &cb) noexcept {
    io_water_cb_ = std::move(cb);
  }
  void SetConnWriteIOCPCb(const WriteIOCPCallback &cb) noexcept {
    write_iocp_cb_ = std::move(cb);
  }
  void SetConnSuccessCb(const ConnSuccCallback &cb) noexcept {
    succ_cb_ = std::move(cb);
  }
  void SetConnReadCb(const ReadCallback &cb) noexcept {
    read_cb_ = std::move(cb);
  }
  void SetConnWriteCb(const WriteCallback &cb) noexcept {
    write_cb_ = std::move(cb);
  }
  void SetConnCloseCb(const CloseCallback &cb) noexcept {
    close_cb_ = std::move(cb);
  }

  void set_buffer_type(BufferType type) noexcept { buffer_type_ = type; }
  BufferType buffer_type() const noexcept { return buffer_type_; }

 protected:
  void SubmitWriteIovecSafe(BufferIovec &queue) noexcept;
  void SubmitWriteIovec(BufferIovec &queue) noexcept;
  void UpdateWriteBusyIovec() noexcept;
  void UpdateWriteBusyOffset() noexcept;
  virtual void CloseConn();
  virtual void ActiveClose();
  void Shutdown(ShutdownMode mode);

 protected:
  int fd_ = -1;
  State state_ = State::kStateNone;

  BufferType buffer_type_;

  Buffer read_buf_;
  Buffer write_buf_;
  // not copy to circular buffer
  // mutex means whether thread sate
  std::mutex *mutex_;
  BufferIovec write_pending_queue_;
  BufferIovec write_busy_queue_;

  EventLoop *loop_;

  HighWaterCallback io_water_cb_;
  WriteIOCPCallback write_iocp_cb_;
  ConnSuccCallback succ_cb_;
  ReadCallback read_cb_;
  WriteCallback write_cb_;
  CloseCallback close_cb_;
};
}

#endif