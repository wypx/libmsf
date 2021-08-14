
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
#ifndef BASE_UPNP_H_
#define BASE_UPNP_H_

#include <natpmp.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace MSF {

typedef enum {
  TR_NATPMP_IDLE,
  TR_NATPMP_ERR,
  TR_NATPMP_DISCOVER,
  TR_NATPMP_RECV_PUB,
  TR_NATPMP_SEND_MAP,
  TR_NATPMP_RECV_MAP,
  TR_NATPMP_SEND_UNMAP,
  TR_NATPMP_RECV_UNMAP
} NatPMPState;

class NatPMP {
 public:
  NatPMP();
  ~NatPMP();

  bool Pulse(uint16_t private_port, bool enabled, uint16_t* public_port);

 private:
  bool CanSendCommand();
  void SetCommandTime();

 private:
  NatPMPState state_;
  bool discovered_ = false;
  bool mapped_;
  uint16_t public_port_ = 0;
  uint16_t private_port_ = 0;
  time_t renew_time_;
  time_t command_time_;
  natpmp_t natpmp_;
};

}  // namespace MSF
#endif