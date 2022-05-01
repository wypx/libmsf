
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
#ifndef SOCK_SSL_CONNECTION_H_
#define SOCK_SSL_CONNECTION_H_

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include "connection.h"
#include "event.h"

namespace MSF {

class SSLConnection : public Connection {
 public:
  SSLConnection(EventLoop *loop, int fd, bool thread_safe = true);
  ~SSLConnection();

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

  void HandleReadEvent() override;
  void HandleWriteEvent() override;
  void HandleCloseEvent() override;
  void HandleSuccEvent() override;

  inline bool IsConnError(int64_t ret);

 private:
  bool connected_ = false;
  SSL *ssl_handle_ = nullptr;
};

}  // namespace MSF
#endif