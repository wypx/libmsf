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
#ifndef LIB_ACCEPTTOR_H_
#define LIB_ACCEPTTOR_H_

#include "Noncopyable.h"
#include "EventLoop.h"
#include "InetAddress.h"

using namespace MSF;

namespace MSF {

typedef std::function<void(const int, const uint16_t)> NewConnCb;

class Acceptor;
typedef std::shared_ptr<Acceptor> AcceptorPtr;

class Acceptor : public Noncopyable {
 public:
  explicit Acceptor(EventLoop *loop, const InetAddress &addr,
                    const NewConnCb &cb);
  ~Acceptor();

  void acceptCb();
  void errorCb();

  bool listen();
  bool enableListen();
  bool disableListen();
  const int sfd() const { return sfd_; }

 private:
  void closeIdleFd();
  void openIdleFd();
  void discardAccept();

 private:
  bool started_;
  bool quit_;
  int idleFd_;
  int backLog_;
  int sfd_;

  EventLoop *loop_;
  InetAddress addr_;
  NewConnCb newConnCb_;
};

}  // namespace MSF
#endif