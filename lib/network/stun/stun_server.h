/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef P2P_BASE_STUN_SERVER_H_
#define P2P_BASE_STUN_SERVER_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>

#include "stun.h"
#include "async_packet_socket.h"
#include "async_udp_socket.h"
#include "socket_address.h"
#include "sigslot.h"

namespace MSF {

const int STUN_SERVER_PORT = 3478;

class StunServer : public has_slots<> {
 public:
  // Creates a STUN server, which will listen on the given socket.
  explicit StunServer(AsyncUDPSocket* socket);
  // Removes the STUN server from the socket and deletes the socket.
  ~StunServer() override;

 protected:
  // Slot for AsyncSocket.PacketRead:
  void OnPacket(AsyncPacketSocket* socket, const char* buf, size_t size,
                const SocketAddress& remote_addr,
                const int64_t& packet_time_us);

  // Handlers for the different types of STUN/TURN requests:
  virtual void OnBindingRequest(StunMessage* msg, const SocketAddress& addr);
  void OnAllocateRequest(StunMessage* msg, const SocketAddress& addr);
  void OnSharedSecretRequest(StunMessage* msg, const SocketAddress& addr);
  void OnSendRequest(StunMessage* msg, const SocketAddress& addr);

  // Sends an error response to the given message back to the user.
  void SendErrorResponse(const StunMessage& msg, const SocketAddress& addr,
                         int error_code, const char* error_desc);

  // Sends the given message to the appropriate destination.
  void SendResponse(const StunMessage& msg, const SocketAddress& addr);

  // A helper method to compose a STUN binding response.
  void GetStunBindResponse(StunMessage* request,
                           const SocketAddress& remote_addr,
                           StunMessage* response) const;

 private:
  std::unique_ptr<AsyncUDPSocket> socket_;
};

}  // namespace cricket

#endif  // P2P_BASE_STUN_SERVER_H_
