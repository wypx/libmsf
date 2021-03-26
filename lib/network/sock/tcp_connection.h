
// /**************************************************************************
//  *
//  * Copyright (c) 2017-2021, luotang.me <wypx520@gmail.com>, China.
//  * All rights reserved.
//  *
//  * Distributed under the terms of the GNU General Public License v2.
//  *
//  * This software is provided 'as is' with no explicit or implied warranties
//  * in respect of its properties, including, but not limited to, correctness
//  * and/or fitness for purpose.
//  *
//  **************************************************************************/

#include "connection.h"
#include "event.h"

namespace MSF {

class TCPConnection : public Connection {
  TCPConnection(EventLoop *loop, int fd, bool thread_safe = true);
  ~TCPConnection();

  // recv in buffer
  size_t ReadableLength() override;
  size_t ReceiveData(void *data, size_t len) override;
  size_t DrainData(size_t len) override;
  size_t RemoveData(void *data, size_t len) override;

  bool IsClosed() override;
  void CloseConn() override;
  void Shutdown(ShutdownMode mode) override;

  void *Malloc(size_t len, size_t align) override;
  void Free(void *ptr) override;

  bool HandleReadEvent() override;
  bool HandleWriteEvent() override;
  void HandleErrorEvent() override;
  void HandleConnectedEvent() override;

  inline bool IsConnError(int64_t ret);

 private:
  Event event_;
};

}  // namespace MSF