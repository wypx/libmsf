/*
 *  Copyright 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "default_ice_transport_factory.h"

#include <utility>

#include "ref_counted_object.h"
#include "basic_ice_controller.h"
#include "ice_controller_factory_interface.h"

namespace {

class BasicIceControllerFactory : public IceControllerFactoryInterface {
 public:
  std::unique_ptr<IceControllerInterface> Create(
      const IceControllerFactoryArgs& args) override {
    return std::make_unique<BasicIceController>(args);
  }
};

}  // namespace

namespace MSF {

DefaultIceTransport::DefaultIceTransport(
    std::unique_ptr<P2PTransportChannel> internal)
    : internal_(std::move(internal)) {}

DefaultIceTransport::~DefaultIceTransport() {
  // RTC_DCHECK_RUN_ON(&thread_checker_);
}

scoped_refptr<IceTransportInterface>
DefaultIceTransportFactory::CreateIceTransport(
    const std::string& transport_name, int component, IceTransportInit init) {
  BasicIceControllerFactory factory;
  return new RefCountedObject<DefaultIceTransport>(
      std::make_unique<P2PTransportChannel>(
          transport_name, component, init.port_allocator(),
          /*init.async_resolver_factory(), init.event_log(),*/ &factory));
}

}  // namespace webrtc
