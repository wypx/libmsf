/*
 *  Copyright 2017 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef P2P_CLIENT_RELAY_PORT_FACTORY_INTERFACE_H_
#define P2P_CLIENT_RELAY_PORT_FACTORY_INTERFACE_H_

#include <memory>
#include <string>

#include "port_interface.h"
#include "ref_count.h"
#include "turn_customizer.h"

namespace MSF {

class EventLoop;
class Port;
struct ProtocolAddress;
struct RelayServerConfig;

// A struct containing arguments to RelayPortFactory::Create()
struct CreateRelayPortArgs {
  CreateRelayPortArgs();
  // Thread* network_thread;
  EventLoop* loop;
  PacketSocketFactory* socket_factory;
  Network* network;
  const ProtocolAddress* server_address;
  const RelayServerConfig* config;
  std::string username;
  std::string password;
  std::string origin;
  TurnCustomizer* turn_customizer;
};

inline CreateRelayPortArgs::CreateRelayPortArgs() {}

// A factory for creating RelayPort's.
class RelayPortFactoryInterface {
 public:
  virtual ~RelayPortFactoryInterface() {}

  // This variant is used for UDP connection to the relay server
  // using a already existing shared socket.
  virtual std::unique_ptr<Port> Create(const CreateRelayPortArgs& args,
                                       AsyncPacketSocket* udp_socket) = 0;

  // This variant is used for the other cases.
  virtual std::unique_ptr<Port> Create(const CreateRelayPortArgs& args,
                                       int min_port, int max_port) = 0;
};

}  // namespace cricket

#endif  // P2P_CLIENT_RELAY_PORT_FACTORY_INTERFACE_H_
