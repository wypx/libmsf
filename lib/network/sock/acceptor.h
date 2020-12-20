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

#include <functional>
#include <memory>

#include "inet_address.h"
#include "noncopyable.h"

using namespace MSF;

namespace MSF {

class EventLoop;

typedef std::function<void(const int, const uint16_t)> NewConnCb;

class Acceptor;
typedef std::shared_ptr<Acceptor> AcceptorPtr;

class Acceptor : public Noncopyable {
 public:
  explicit Acceptor(const InetAddress &addr, const NewConnCb &cb);
  ~Acceptor();

  void AcceptCb();
  void ErrorCb();

  bool EnableListen();
  bool DisableListen();
  const int sfd() const { return sfd_; }

 private:
  bool StartListen();
  bool StopListen();
  bool HandleAcceptError(int err);
  void CloseIdleFd();
  void OpenIdleFd();
  void PopAcceptQueue();
  void DiscardAccept();

 private:
  bool started_;
  int idle_fd_;
  int back_log_;
  int sfd_;

  InetAddress addr_;
  NewConnCb new_conn_cb_;

  // EventLoop* const loop_; // which loop belong to
};

}  // namespace MSF
#endif