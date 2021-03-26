
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
#include "latch.h"
#include "callback.h"

namespace MSF {

class ProxyConnection {
 public:
 private:
  ConnectionPtr conn_;
};

class ProxyPDU {
 public:
  bool WaitAck(const uint32_t ts) { return latch_.WaitFor(ts); }
  void PostAck() { return latch_.CountDown(); }

 private:
  uint16_t remote_id_;
  uint16_t local_id_;
  uint32_t command_;

  CountDownLatch latch_;
};

}  // namespace MSF