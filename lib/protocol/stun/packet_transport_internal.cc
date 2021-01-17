/*
 *  Copyright 2017 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "packet_transport_internal.h"

namespace MSF {

PacketTransportInternal::PacketTransportInternal() = default;

PacketTransportInternal::~PacketTransportInternal() = default;

bool PacketTransportInternal::GetOption(Socket::Option opt, int* value) {
  return false;
}

std::optional<NetworkRoute> PacketTransportInternal::network_route() const {
  return std::optional<NetworkRoute>();
}

}  // namespace rtc
