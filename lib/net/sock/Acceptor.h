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

#include "InetAddress.h"
#include "Noncopyable.h"

using namespace MSF;

namespace MSF {

typedef std::function<void(const int, const uint16_t)> NewConnCb;

class Acceptor;
typedef std::shared_ptr<Acceptor> AcceptorPtr;

class Acceptor : public Noncopyable {
 public:
  explicit Acceptor(const InetAddress &addr, const NewConnCb &cb);
  ~Acceptor();

  void AcceptCb();
  void ErrorCb();

  bool StartListen();
  bool EnableListen();
  bool DisableListen();
  const int sfd() const { return sfd_; }

 private:
  void CloseIdleFd();
  void OpenIdleFd();
  void DiscardAccept();
 private:
  bool started_;
  int idle_fd_;
  int back_log_;
  int sfd_;

  InetAddress addr_;
  NewConnCb new_conn_cb_;
};

}  // namespace MSF
#endif