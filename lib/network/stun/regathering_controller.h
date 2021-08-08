/*
 *  Copyright 2018 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef P2P_BASE_REGATHERING_CONTROLLER_H_
#define P2P_BASE_REGATHERING_CONTROLLER_H_

#include "ice_transport_internal.h"
#include "port_allocator.h"

namespace MSF {

// Controls regathering of candidates for the ICE transport passed into it,
// reacting to signals like SignalWritableState, SignalNetworkRouteChange, etc.,
// using methods like GetStats to get additional information, and calling
// methods like RegatherOnFailedNetworks on the PortAllocatorSession when
// regathering is desired.
//
// "Regathering" is defined as gathering additional candidates within a single
// ICE generation (or in other words, PortAllocatorSession), and is possible
// when "continual gathering" is enabled. This may allow connectivity to be
// maintained and/or restored without a full ICE restart.
//
// Regathering will only begin after PortAllocationSession is set via
// set_allocator_session. This should be called any time the "active"
// PortAllocatorSession is changed (in other words, when an ICE restart occurs),
// so that candidates are gathered for the "current" ICE generation.
//
// All methods of BasicRegatheringController should be called on the same
// thread as the one passed to the constructor, and this thread should be the
// same one where PortAllocatorSession runs, which is also identical to the
// network thread of the ICE transport, as given by
// P2PTransportChannel::thread().
class BasicRegatheringController : public has_slots<> {
 public:
  struct Config {
    int regather_on_failed_networks_interval =
        REGATHER_ON_FAILED_NETWORKS_INTERVAL;
  };

  BasicRegatheringController() = delete;
  BasicRegatheringController(const Config& config,
                             IceTransportInternal* ice_transport,
                             EventLoop* loop);
  ~BasicRegatheringController() override;
  // TODO(qingsi): Remove this method after implementing a new signal in
  // P2PTransportChannel and reacting to that signal for the initial schedules
  // of regathering.
  void Start();
  void set_allocator_session(PortAllocatorSession* allocator_session) {
    allocator_session_ = allocator_session;
  }
  // Setting a different config of the regathering interval range on all
  // networks cancels and reschedules the recurring schedules, if any, of
  // regathering on all networks. The same applies to the change of the
  // regathering interval on the failed networks. This rescheduling behavior is
  // seperately defined for the two config parameters.
  void SetConfig(const Config& config);

 private:
  // TODO(qingsi): Implement the following methods and use methods from the ICE
  // transport like GetStats to get additional information for the decision
  // making in regathering.
  void OnIceTransportStateChanged(IceTransportInternal*) {}
  void OnIceTransportWritableState(PacketTransportInternal*) {}
  void OnIceTransportReceivingState(PacketTransportInternal*) {}
  void OnIceTransportNetworkRouteChanged(std::optional<NetworkRoute>) {}
  // Schedules delayed and repeated regathering of local candidates on failed
  // networks, where the delay in milliseconds is given by the config. Each
  // repetition is separated by the same delay. When scheduled, all previous
  // schedules are canceled.
  void ScheduleRecurringRegatheringOnFailedNetworks();
  // Cancels regathering scheduled by ScheduleRecurringRegatheringOnAllNetworks.
  void CancelScheduledRecurringRegatheringOnAllNetworks();
  // Cancels regathering scheduled by
  // ScheduleRecurringRegatheringOnFailedNetworks.
  void CancelScheduledRecurringRegatheringOnFailedNetworks();

  // The following method perform the actual regathering, if the recent port
  // allocator session has done the initial gathering.
  void RegatherOnFailedNetworksIfDoneGathering();

  Config config_;
  IceTransportInternal* ice_transport_;
  PortAllocatorSession* allocator_session_ = nullptr;
  bool has_recurring_schedule_on_failed_networks_ = false;
  // Thread* thread_;
  EventLoop* loop_;
  // AsyncInvoker invoker_for_failed_networks_;
};

}  // namespace webrtc

#endif  // P2P_BASE_REGATHERING_CONTROLLER_H_
