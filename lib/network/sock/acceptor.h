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
#ifndef SOCK_ACCEPTTOR_H_
#define SOCK_ACCEPTTOR_H_

#include <network/sock/callback.h>
#include <network/sock/inet_address.h>
#include <network/sock/sock_utils.h>

#include <functional>
#include <memory>

#include "event.h"

using namespace MSF;

namespace MSF {

class EventLoop;

class Acceptor : public noncopyable {
 public:
  explicit Acceptor(EventLoop *loop, const InetAddress &addr,
                    const NewConnCallback &cb);
  ~Acceptor();

  bool Start();
  void Stop();
  void OpenListen();
  void CloseListen();

  EventLoop *loop() { return loop_; }

 private:
  void AcceptCb();
  void ErrorCb();
  bool HandleError(int error);
  void CloseAccept();
  void CloseIdleFd();
  void OpenIdleFd();
  void PopAcceptQueue();
  void DiscardAccept();

 private:
  EventLoop *loop_;

  static const int kAcceptorBacklog = 8;
  int back_log_ = kAcceptorBacklog;
  int sfd_ = kInvalidSocket;
  int idle_fd_ = kInvalidSocket;

  std::unique_ptr<Event> event_;
  InetAddress addr_;
  NewConnCallback new_conn_cb_;
};

}  // namespace MSF
#endif