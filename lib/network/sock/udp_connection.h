
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
#ifndef SOCK_TCP_CONNECTION_H_
#define SOCK_TCP_CONNECTION_H_

#include "connection.h"

namespace MSF {

class UDPConnection : public Connection {
  UDPConnection(EventLoop *loop, int fd, bool thread_safe = true);
  ~UDPConnection();

  bool HandleReadEvent() override;
  bool HandleWriteEvent() override;
  void HandleErrorEvent() override;

  void CloseConn() override;
  void Shutdown(ShutdownMode mode);
};

}  // namespace MSF
#endif