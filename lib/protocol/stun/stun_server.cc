/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "stun_server.h"

#include <utility>
#include <butil/logging.h>

#include "byte_buffer.h"

namespace MSF {

StunServer::StunServer(AsyncUDPSocket* socket) : socket_(socket) {
  socket_->SignalReadPacket.connect(this, &StunServer::OnPacket);
}

StunServer::~StunServer() { socket_->SignalReadPacket.disconnect(this); }

void StunServer::OnPacket(AsyncPacketSocket* socket, const char* buf,
                          size_t size, const SocketAddress& remote_addr,
                          const int64_t& /* packet_time_us */) {
  // Parse the STUN message; eat any messages that fail to parse.
  ByteBufferReader bbuf(buf, size);
  StunMessage msg;
  if (!msg.Read(&bbuf)) {
    return;
  }

  // TODO(?): If unknown non-optional (<= 0x7fff) attributes are found, send a
  //          420 "Unknown Attribute" response.

  // Send the message to the appropriate handler function.
  switch (msg.type()) {
    case kStunMessageBindingRequest:
      OnBindingRequest(&msg, remote_addr);
      break;

    default:
      SendErrorResponse(msg, remote_addr, 600, "Operation Not Supported");
  }
}

void StunServer::OnBindingRequest(StunMessage* msg,
                                  const SocketAddress& remote_addr) {
  StunMessage response;
  GetStunBindResponse(msg, remote_addr, &response);
  SendResponse(response, remote_addr);
}

void StunServer::SendErrorResponse(const StunMessage& msg,
                                   const SocketAddress& addr, int error_code,
                                   const char* error_desc) {
  StunMessage err_msg;
  err_msg.SetType(GetStunErrorResponseType(msg.type()));
  err_msg.SetTransactionID(msg.transaction_id());

  auto err_code = StunAttribute::CreateErrorCode();
  err_code->SetCode(error_code);
  err_code->SetReason(error_desc);
  err_msg.AddAttribute(std::move(err_code));

  SendResponse(err_msg, addr);
}

void StunServer::SendResponse(const StunMessage& msg,
                              const SocketAddress& addr) {
  ByteBufferWriter buf;
  msg.Write(&buf);
  PacketOptions options;
  if (socket_->SendTo(buf.Data(), buf.Length(), addr, options) < 0)
    LOG(ERROR) << "sendto";
}

void StunServer::GetStunBindResponse(StunMessage* request,
                                     const SocketAddress& remote_addr,
                                     StunMessage* response) const {
  response->SetType(kStunMessageBindingResponse);
  response->SetTransactionID(request->transaction_id());

  // Tell the user the address that we received their request from.
  std::unique_ptr<StunAddressAttribute> mapped_addr;
  if (request->IsLegacy()) {
    mapped_addr = StunAttribute::CreateAddress(kStunAttrMappedAddress);
  } else {
    mapped_addr = StunAttribute::CreateXorAddress(kStunAttrXorMappedAddress);
  }
  mapped_addr->SetAddress(remote_addr);
  response->AddAttribute(std::move(mapped_addr));
}

}  // namespace cricket
