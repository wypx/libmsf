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
#include "event.h"
#include "sock_utils.h"

using namespace MSF;

namespace MSF {

enum ShutdownMode {
  SHUTDOWN_BOTH,
  SHUTDOWN_READ,
  SHUTDOWN_WRITE,
};

class Connection : public std::enable_shared_from_this<Connection> {
 public:
  enum State {
    kStateNone,
    kStateConnected,
    kStateCloseWaitWrite,  // passive or active close, but I still have data to
                           // send
    kStatePassiveClose,    // should close
    kStateActiveClose,     // should close
    kStateError,           // waiting to handle error
    kStateClosed,          // final state being to destroy
  };

  Connection();
  ~Connection();

  bool HandleReadEvent();
  bool HandleWriteEvent();
  void HandleErrorEvent();

 protected:
  void CloseConn();
  void ActiveClose();
  void Shutdown(ShutdownMode mode);

 protected:
  int fd_;
  Buffer send_buf_;
  Buffer recv_buf_;

  State state_ = State::kStateNone;
};
}

#endif