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
#include "natmap.h"

#include <arpa/inet.h>
#include <base/logging.h>

namespace MSF {

#define LIFETIME_SECS 3600
#define COMMAND_WAIT_SECS 8

NatPMP::NatPMP() {}

NatPMP::~NatPMP() { ::closenatpmp(&natpmp_); }

bool NatPMP::CanSendCommand() { return ::time(nullptr) >= command_time_; }

void NatPMP::SetCommandTime() {
  command_time_ = ::time(nullptr) + COMMAND_WAIT_SECS;
}

bool NatPMP::Pulse(uint16_t private_port, bool enabled, uint16_t* public_port) {
  if (enabled && (state_ == TR_NATPMP_DISCOVER)) {
    int ret = ::initnatpmp(&natpmp_, 0, 0);
    ret = ::sendpublicaddressrequest(&natpmp_);
    state_ = ret < 0 ? TR_NATPMP_ERR : TR_NATPMP_RECV_PUB;
    discovered_ = true;
    SetCommandTime();
  }

  if ((state_ == TR_NATPMP_RECV_PUB) && CanSendCommand()) {
    natpmpresp_t response;
    const int ret = ::readnatpmpresponseorretry(&natpmp_, &response);
    if (ret >= 0) {
      char str[128];
      ::inet_ntop(AF_INET, &response.pnu.publicaddress.addr, str, sizeof(str));
      LOG(INFO) << "found public address " << str;
      state_ = TR_NATPMP_IDLE;
    } else if (ret != NATPMP_TRYAGAIN) {
      state_ = TR_NATPMP_ERR;
    }
  }

  if ((state_ == TR_NATPMP_IDLE) || (state_ == TR_NATPMP_ERR)) {
    if (mapped_ && (!enabled || (private_port_ != private_port)))
      state_ = TR_NATPMP_SEND_UNMAP;
  }

  if ((state_ == TR_NATPMP_SEND_UNMAP) && CanSendCommand()) {
    const int ret = ::sendnewportmappingrequest(&natpmp_, NATPMP_PROTOCOL_TCP,
                                                private_port_, public_port_, 0);
    state_ = ret < 0 ? TR_NATPMP_ERR : TR_NATPMP_RECV_UNMAP;
    SetCommandTime();
  }

  if (state_ == TR_NATPMP_RECV_UNMAP) {
    natpmpresp_t resp;
    const int ret = ::readnatpmpresponseorretry(&natpmp_, &resp);
    if (ret >= 0) {
      const int private_port = resp.pnu.newportmapping.privateport;

      LOG(INFO) << "no longer forwarding port " << private_port;

      if (private_port_ == private_port) {
        private_port_ = 0;
        public_port_ = 0;
        state_ = TR_NATPMP_IDLE;
        mapped_ = false;
      }
    } else if (ret != NATPMP_TRYAGAIN) {
      state_ = TR_NATPMP_ERR;
    }
  }

  if (state_ == TR_NATPMP_IDLE) {
    if (enabled && !mapped_ && discovered_)
      state_ = TR_NATPMP_SEND_MAP;

    else if (mapped_ && ::time(nullptr) >= renew_time_)
      state_ = TR_NATPMP_SEND_MAP;
  }

  if ((state_ == TR_NATPMP_SEND_MAP) && CanSendCommand()) {
    const int ret = ::sendnewportmappingrequest(&natpmp_, NATPMP_PROTOCOL_TCP,
                                                private_port_, private_port_,
                                                LIFETIME_SECS);
    state_ = ret < 0 ? TR_NATPMP_ERR : TR_NATPMP_RECV_MAP;
    SetCommandTime();
  }

  if (state_ == TR_NATPMP_RECV_MAP) {
    natpmpresp_t resp;
    const int ret = ::readnatpmpresponseorretry(&natpmp_, &resp);
    if (ret >= 0) {
      state_ = TR_NATPMP_IDLE;
      mapped_ = true;
      renew_time_ = ::time(nullptr) + (resp.pnu.newportmapping.lifetime / 2);
      private_port_ = resp.pnu.newportmapping.privateport;
      public_port_ = resp.pnu.newportmapping.mappedpublicport;
      LOG(INFO) << "port " << private_port << " forwarded successfully";
    } else if (ret != NATPMP_TRYAGAIN) {
      state_ = TR_NATPMP_ERR;
    }
  }
  *public_port = public_port_;
  return true;
}

}  // namespace MSF