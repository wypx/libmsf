/*
 *  Copyright 2011 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef P2P_BASE_BASIC_PACKET_SOCKET_FACTORY_H_
#define P2P_BASE_BASIC_PACKET_SOCKET_FACTORY_H_

#include <string>

#include "packet_socket_factory.h"
#include "event_loop.h"

namespace MSF {

class AsyncSocket;
class SocketFactory;
class EventLoop;

class BasicPacketSocketFactory : public PacketSocketFactory {
 public:
  BasicPacketSocketFactory(EventLoop* loop);
  explicit BasicPacketSocketFactory(EventLoop* loop,
                                    SocketFactory* socket_factory);
  ~BasicPacketSocketFactory() override;

  AsyncPacketSocket* CreateUdpSocket(const SocketAddress& local_address,
                                     uint16_t min_port,
                                     uint16_t max_port) override;
  AsyncPacketSocket* CreateServerTcpSocket(const SocketAddress& local_address,
                                           uint16_t min_port, uint16_t max_port,
                                           int opts) override;

  EventLoop* event_loop() { return loop_; }
  AsyncPacketSocket* CreateClientTcpSocket(
      const SocketAddress& local_address, const SocketAddress& remote_address,
      const ProxyInfo& proxy_info, const std::string& user_agent,
      const PacketSocketTcpOptions& tcp_options) override;

  // AsyncResolverInterface* CreateAsyncResolver() override;

 private:
  int BindSocket(AsyncSocket* socket, const SocketAddress& local_address,
                 uint16_t min_port, uint16_t max_port);

  SocketFactory* socket_factory();

  EventLoop* loop_;
  SocketFactory* socket_factory_;
  std::unique_ptr<SocketFactory> internal_socket_factory_;
};

}  // namespace rtc

#endif  // P2P_BASE_BASIC_PACKET_SOCKET_FACTORY_H_
