/*
 *  Copyright 2015 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "network_monitor.h"

#include <stdint.h>

#include "location.h"
#include <base/logging.h>

namespace MSF {

const uint32_t UPDATE_NETWORKS_MESSAGE = 1;

// This is set by NetworkMonitorFactory::SetFactory and the caller of
// NetworkMonitorFactory::SetFactory must be responsible for calling
// ReleaseFactory to destroy the factory.
NetworkMonitorFactory* network_monitor_factory = nullptr;

NetworkMonitorInterface::NetworkMonitorInterface() {}

NetworkMonitorInterface::~NetworkMonitorInterface() {}

NetworkMonitorBase::NetworkMonitorBase() : loop_(nullptr) {}
NetworkMonitorBase::~NetworkMonitorBase() {}

void NetworkMonitorBase::OnNetworksChanged() {
  LOG(TRACE) << "Network change is received at the network monitor";
  // worker_thread_->Post(RTC_FROM_HERE, this, UPDATE_NETWORKS_MESSAGE);
}

// void NetworkMonitorBase::OnMessage(Message* msg) {
//   assert(msg->message_id == UPDATE_NETWORKS_MESSAGE);
//   SignalNetworksChanged();
// }

AdapterType NetworkMonitorBase::GetVpnUnderlyingAdapterType(
    const std::string& interface_name) {
  return ADAPTER_TYPE_UNKNOWN;
}

NetworkMonitorFactory::NetworkMonitorFactory() {}
NetworkMonitorFactory::~NetworkMonitorFactory() {}

void NetworkMonitorFactory::SetFactory(NetworkMonitorFactory* factory) {
  if (network_monitor_factory != nullptr) {
    delete network_monitor_factory;
  }
  network_monitor_factory = factory;
}

void NetworkMonitorFactory::ReleaseFactory(NetworkMonitorFactory* factory) {
  if (factory == network_monitor_factory) {
    SetFactory(nullptr);
  }
}

NetworkMonitorFactory* NetworkMonitorFactory::GetFactory() {
  return network_monitor_factory;
}

}  // namespace MSF
