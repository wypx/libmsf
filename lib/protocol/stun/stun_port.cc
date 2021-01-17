/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "stun_port.h"

#include <utility>
#include <vector>
#include <sstream>
#include <butil/logging.h>

#include "stun.h"
#include "connection.h"
#include "p2p_constants.h"
#include "port_allocator.h"
#include "helpers.h"
#include "ip_address.h"
#include "net_helpers.h"

namespace MSF {

// TODO(?): Move these to a common place (used in relayport too)
const int RETRY_TIMEOUT = 50 * 1000;  // 50 seconds

// Stop logging errors in UDPPort::SendTo after we have logged
// |kSendErrorLogLimit| messages. Start again after a successful send.
const int kSendErrorLogLimit = 5;

class EventLoop;
// Handles a binding request sent to the STUN server.
class StunBindingRequest : public StunRequest {
 public:
  StunBindingRequest(UDPPort* port, const SocketAddress& addr,
                     int64_t start_time)
      : port_(port), server_addr_(addr), start_time_(start_time) {}

  const SocketAddress& server_addr() const { return server_addr_; }

  void Prepare(StunMessage* request) override {
    request->SetType(kStunMessageBindingRequest);
  }

  void OnResponse(StunMessage* response) override {
    const StunAddressAttribute* addr_attr =
        response->GetAddress(kStunAttrMappedAddress);
    if (!addr_attr) {
      LOG(ERROR) << "Binding response missing mapped address.";
    } else if (addr_attr->family() != kStunAddressIpv4 &&
               addr_attr->family() != kStunAddressIpv6) {
      LOG(ERROR) << "Binding address has bad family";
    } else {
      SocketAddress addr(addr_attr->ipaddr(), addr_attr->port());
      port_->OnStunBindingRequestSucceeded(this->Elapsed(), server_addr_, addr);
    }

    // The keep-alive requests will be stopped after its lifetime has passed.
    if (WithinLifetime(TimeMillis())) {
      port_->requests_.SendDelayed(
          new StunBindingRequest(port_, server_addr_, start_time_),
          port_->stun_keepalive_delay());
    }
  }

  void OnErrorResponse(StunMessage* response) override {
    const StunErrorCodeAttribute* attr = response->GetErrorCode();
    if (!attr) {
      LOG(ERROR) << "Missing binding response error code.";
    } else {
      LOG(ERROR) << "Binding error response:"
                    " class=" << attr->eclass() << " number=" << attr->number()
                 << " reason=" << attr->reason();
    }

    port_->OnStunBindingOrResolveRequestFailed(
        server_addr_, attr ? attr->number() : kStunGlobalFailure,
        attr ? attr->reason()
             : "STUN binding response with no error code attribute.");

    int64_t now = TimeMillis();
    if (WithinLifetime(now) && TimeDiff(now, start_time_) < RETRY_TIMEOUT) {
      port_->requests_.SendDelayed(
          new StunBindingRequest(port_, server_addr_, start_time_),
          port_->stun_keepalive_delay());
    }
  }
  void OnTimeout() override {
    LOG(ERROR) << "Binding request timed out from "
               << port_->GetLocalAddress().ToSensitiveString() << " ("
               << port_->network()->name() << ")";
    port_->OnStunBindingOrResolveRequestFailed(
        server_addr_, kStunServerNotReachable,
        "STUN allocate request timed out.");
  }

 private:
  // Returns true if |now| is within the lifetime of the request (a negative
  // lifetime means infinite).
  bool WithinLifetime(int64_t now) const {
    int lifetime = port_->stun_keepalive_lifetime();
    return lifetime < 0 || TimeDiff(now, start_time_) <= lifetime;
  }

  UDPPort* port_;
  const SocketAddress server_addr_;

  int64_t start_time_;
};

#if 0
UDPPort::AddressResolver::AddressResolver(PacketSocketFactory* factory)
    : socket_factory_(factory) {}

UDPPort::AddressResolver::~AddressResolver() {
  for (ResolverMap::iterator it = resolvers_.begin(); it != resolvers_.end();
       ++it) {
    // TODO(guoweis): Change to asynchronous DNS resolution to prevent the hang
    // when passing true to the Destroy() which is a safer way to avoid the code
    // unloaded before the thread exits. Please see webrtc bug 5139.
    it->second->Destroy(false);
  }
}

void UDPPort::AddressResolver::Resolve(const SocketAddress& address) {
  if (resolvers_.find(address) != resolvers_.end()) return;

  AsyncResolverInterface* resolver = socket_factory_->CreateAsyncResolver();
  resolvers_.insert(
      std::pair<SocketAddress, AsyncResolverInterface*>(address, resolver));

  resolver->SignalDone.connect(this,
                               &UDPPort::AddressResolver::OnResolveResult);

  resolver->Start(address);
}

bool UDPPort::AddressResolver::GetResolvedAddress(const SocketAddress& input,
                                                  int family,
                                                  SocketAddress* output) const {
  ResolverMap::const_iterator it = resolvers_.find(input);
  if (it == resolvers_.end()) return false;

  return it->second->GetResolvedAddress(family, output);
}

void UDPPort::AddressResolver::OnResolveResult(
    AsyncResolverInterface* resolver) {
  for (ResolverMap::iterator it = resolvers_.begin(); it != resolvers_.end();
       ++it) {
    if (it->second == resolver) {
      SignalDone(it->first, resolver->GetError());
      return;
    }
  }
}
#endif

UDPPort::UDPPort(PacketSocketFactory* factory, Network* network,
                 AsyncPacketSocket* socket, const std::string& username,
                 const std::string& password, const std::string& origin,
                 bool emit_local_for_anyaddress)
    : Port(LOCAL_PORT_TYPE, factory, network, username, password),
      requests_(nullptr),
      socket_(socket),
      error_(0),
      ready_(false),
      stun_keepalive_delay_(STUN_KEEPALIVE_INTERVAL),
      dscp_(DSCP_NO_CHANGE),
      emit_local_for_anyaddress_(emit_local_for_anyaddress) {
  requests_.set_origin(origin);
}

UDPPort::UDPPort(PacketSocketFactory* factory, Network* network,
                 uint16_t min_port, uint16_t max_port,
                 const std::string& username, const std::string& password,
                 const std::string& origin, bool emit_local_for_anyaddress)
    : Port(LOCAL_PORT_TYPE, factory, network, min_port, max_port, username,
           password),
      requests_(nullptr),
      socket_(nullptr),
      error_(0),
      ready_(false),
      stun_keepalive_delay_(STUN_KEEPALIVE_INTERVAL),
      dscp_(DSCP_NO_CHANGE),
      emit_local_for_anyaddress_(emit_local_for_anyaddress) {
  requests_.set_origin(origin);
}

bool UDPPort::Init() {
  stun_keepalive_lifetime_ = GetStunKeepaliveLifetime();
  if (!SharedSocket()) {
    assert(socket_ == nullptr);
    socket_ = socket_factory()->CreateUdpSocket(
        SocketAddress(network()->GetBestIP(), 0), min_port(), max_port());
    if (!socket_) {
      LOG(WARNING) << ToString() << ": UDP socket creation failed";
      return false;
    }
    socket_->SignalReadPacket.connect(this, &UDPPort::OnReadPacket);
  }
  socket_->SignalSentPacket.connect(this, &UDPPort::OnSentPacket);
  socket_->SignalReadyToSend.connect(this, &UDPPort::OnReadyToSend);
  socket_->SignalAddressReady.connect(this, &UDPPort::OnLocalAddressReady);
  requests_.SignalSendPacket.connect(this, &UDPPort::OnSendPacket);
  return true;
}

UDPPort::~UDPPort() {
  if (!SharedSocket()) delete socket_;
}

void UDPPort::PrepareAddress() {
  assert(requests_.empty());
  if (socket_->GetState() == AsyncPacketSocket::STATE_BOUND) {
    OnLocalAddressReady(socket_, socket_->GetLocalAddress());
  }
}

void UDPPort::MaybePrepareStunCandidate() {
  // Sending binding request to the STUN server if address is available to
  // prepare STUN candidate.
  if (!server_addresses_.empty()) {
    SendStunBindingRequests();
  } else {
    // Port is done allocating candidates.
    MaybeSetPortCompleteOrError();
  }
}

Connection* UDPPort::CreateConnection(const Candidate& address,
                                      CandidateOrigin origin) {
  if (!SupportsProtocol(address.protocol())) {
    return nullptr;
  }

  if (!IsCompatibleAddress(address.address())) {
    return nullptr;
  }

  // In addition to DCHECK-ing the non-emptiness of local candidates, we also
  // skip this Port with null if there are latent bugs to violate it; otherwise
  // it would lead to a crash when accessing the local candidate of the
  // connection that would be created below.
  if (Candidates().empty()) {
    // RTC_NOTREACHED();
    return nullptr;
  }
  // When the socket is shared, the srflx candidate is gathered by the UDPPort.
  // The assumption here is that
  //  1) if the IP concealment with mDNS is not enabled, the gathering of the
  //     host candidate of this port (which is synchronous),
  //  2) or otherwise if enabled, the start of name registration of the host
  //     candidate (as the start of asynchronous gathering)
  // is always before the gathering of a srflx candidate (and any prflx
  // candidate).
  //
  // See also the definition of MdnsNameRegistrationStatus::kNotStarted in
  // port.h.
  assert(!SharedSocket() || Candidates()[0].type() == LOCAL_PORT_TYPE ||
         mdns_name_registration_status() !=
             MdnsNameRegistrationStatus::kNotStarted);

  Connection* conn = new ProxyConnection(this, 0, address);
  AddOrReplaceConnection(conn);
  return conn;
}

int UDPPort::SendTo(const void* data, size_t size, const SocketAddress& addr,
                    const PacketOptions& options, bool payload) {
  PacketOptions modified_options(options);
  CopyPortInformationToPacketInfo(&modified_options.info_signaled_after_sent);
  int sent = socket_->SendTo(data, size, addr, modified_options);
  if (sent < 0) {
    error_ = socket_->GetError();
    // Rate limiting added for crbug.com/856088.
    // TODO(webrtc:9622): Use general rate limiting mechanism once it exists.
    if (send_error_count_ < kSendErrorLogLimit) {
      ++send_error_count_;
      LOG(ERROR) << ToString() << ": UDP send of " << size
                 << " bytes failed with error " << error_;
    }
  } else {
    send_error_count_ = 0;
  }
  return sent;
}

void UDPPort::UpdateNetworkCost() {
  Port::UpdateNetworkCost();
  stun_keepalive_lifetime_ = GetStunKeepaliveLifetime();
}

DiffServCodePoint UDPPort::StunDscpValue() const { return dscp_; }

int UDPPort::SetOption(Socket::Option opt, int value) {
  if (opt == Socket::OPT_DSCP) {
    // Save value for future packets we instantiate.
    dscp_ = static_cast<DiffServCodePoint>(value);
  }
  return socket_->SetOption(opt, value);
}

int UDPPort::GetOption(Socket::Option opt, int* value) {
  return socket_->GetOption(opt, value);
}

int UDPPort::GetError() { return error_; }

bool UDPPort::HandleIncomingPacket(AsyncPacketSocket* socket, const char* data,
                                   size_t size,
                                   const SocketAddress& remote_addr,
                                   int64_t packet_time_us) {
  // All packets given to UDP port will be consumed.
  OnReadPacket(socket, data, size, remote_addr, packet_time_us);
  return true;
}

bool UDPPort::SupportsProtocol(const std::string& protocol) const {
  return protocol == UDP_PROTOCOL_NAME;
}

ProtocolType UDPPort::GetProtocol() const { return PROTO_UDP; }

void UDPPort::GetStunStats(std::optional<StunStats>* stats) { *stats = stats_; }

void UDPPort::set_stun_keepalive_delay(const std::optional<int>& delay) {
  stun_keepalive_delay_ = delay.value_or(STUN_KEEPALIVE_INTERVAL);
}

void UDPPort::OnLocalAddressReady(AsyncPacketSocket* socket,
                                  const SocketAddress& address) {
  // When adapter enumeration is disabled and binding to the any address, the
  // default local address will be issued as a candidate instead if
  // |emit_local_for_anyaddress| is true. This is to allow connectivity for
  // applications which absolutely requires a HOST candidate.
  SocketAddress addr = address;

  // If MaybeSetDefaultLocalAddress fails, we keep the "any" IP so that at
  // least the port is listening.
  MaybeSetDefaultLocalAddress(&addr);

  AddAddress(addr, addr, SocketAddress(), UDP_PROTOCOL_NAME, "", "",
             LOCAL_PORT_TYPE, ICE_TYPE_PREFERENCE_HOST, 0, "", false);
  MaybePrepareStunCandidate();
}

void UDPPort::PostAddAddress(bool is_final) { MaybeSetPortCompleteOrError(); }

void UDPPort::OnReadPacket(AsyncPacketSocket* socket, const char* data,
                           size_t size, const SocketAddress& remote_addr,
                           const int64_t& packet_time_us) {
  assert(socket == socket_);
  assert(!remote_addr.IsUnresolvedIP());

  // Look for a response from the STUN server.
  // Even if the response doesn't match one of our outstanding requests, we
  // will eat it because it might be a response to a retransmitted packet, and
  // we already cleared the request when we got the first response.
  if (server_addresses_.find(remote_addr) != server_addresses_.end()) {
    requests_.CheckResponse(data, size);
    return;
  }

  if (Connection* conn = GetConnection(remote_addr)) {
    conn->OnReadPacket(data, size, packet_time_us);
  } else {
    Port::OnReadPacket(data, size, remote_addr, PROTO_UDP);
  }
}

void UDPPort::OnSentPacket(AsyncPacketSocket* socket,
                           const SentPacket& sent_packet) {
  PortInterface::SignalSentPacket(sent_packet);
}

void UDPPort::OnReadyToSend(AsyncPacketSocket* socket) {
  Port::OnReadyToSend();
}

void UDPPort::SendStunBindingRequests() {
  // We will keep pinging the stun server to make sure our NAT pin-hole stays
  // open until the deadline (specified in SendStunBindingRequest).
  assert(requests_.empty());

  for (ServerAddresses::const_iterator it = server_addresses_.begin();
       it != server_addresses_.end(); ++it) {
    SendStunBindingRequest(*it);
  }
}

void UDPPort::ResolveStunAddress(const SocketAddress& stun_addr) {
#if 0
  if (!resolver_) {
    resolver_.reset(new AddressResolver(socket_factory()));
    resolver_->SignalDone.connect(this, &UDPPort::OnResolveResult);
  }

  LOG(INFO) << ToString() << ": Starting STUN host lookup for "
            << stun_addr.ToSensitiveString();
  resolver_->Resolve(stun_addr);
#endif
}

void UDPPort::OnResolveResult(const SocketAddress& input, int error) {
#if 0
  assert(resolver_.get() != nullptr);

  SocketAddress resolved;
  if (error != 0 || !resolver_->GetResolvedAddress(
                         input, network()->GetBestIP().family(), &resolved)) {
    LOG(WARNING) << ToString() << ": StunPort: stun host lookup received error "
                 << error;
    OnStunBindingOrResolveRequestFailed(input, kStunServerNotReachable,
                                        "STUN host lookup received error.");
    return;
  }

  server_addresses_.erase(input);

  if (server_addresses_.find(resolved) == server_addresses_.end()) {
    server_addresses_.insert(resolved);
    SendStunBindingRequest(resolved);
  }
#endif
}

void UDPPort::SendStunBindingRequest(const SocketAddress& stun_addr) {
#if 0
  if (stun_addr.IsUnresolvedIP()) {
    ResolveStunAddress(stun_addr);

  } else if (socket_->GetState() == AsyncPacketSocket::STATE_BOUND) {
    // Check if |server_addr_| is compatible with the port's ip.
    if (IsCompatibleAddress(stun_addr)) {
      requests_.Send(new StunBindingRequest(this, stun_addr, TimeMillis()));
    } else {
      // Since we can't send stun messages to the server, we should mark this
      // port ready.
      const char* reason = "STUN server address is incompatible.";
      LOG(WARNING) << reason;
      OnStunBindingOrResolveRequestFailed(stun_addr, kStunServerNotReachable,
                                          reason);
    }
  }
#endif
}

bool UDPPort::MaybeSetDefaultLocalAddress(SocketAddress* addr) const {
  if (!addr->IsAnyIP() || !emit_local_for_anyaddress_ ||
      !network()->default_local_address_provider()) {
    return true;
  }
  IPAddress default_address;
  bool result =
      network()->default_local_address_provider()->GetDefaultLocalAddress(
          addr->family(), &default_address);
  if (!result || default_address.IsNil()) {
    return false;
  }

  addr->SetIP(default_address);
  return true;
}

void UDPPort::OnStunBindingRequestSucceeded(
    int rtt_ms, const SocketAddress& stun_server_addr,
    const SocketAddress& stun_reflected_addr) {
  assert(stats_.stun_binding_responses_received <
         stats_.stun_binding_requests_sent);
  stats_.stun_binding_responses_received++;
  stats_.stun_binding_rtt_ms_total += rtt_ms;
  stats_.stun_binding_rtt_ms_squared_total += rtt_ms * rtt_ms;
  if (bind_request_succeeded_servers_.find(stun_server_addr) !=
      bind_request_succeeded_servers_.end()) {
    return;
  }
  bind_request_succeeded_servers_.insert(stun_server_addr);
  // If socket is shared and |stun_reflected_addr| is equal to local socket
  // address, or if the same address has been added by another STUN server,
  // then discarding the stun address.
  // For STUN, related address is the local socket address.
  if ((!SharedSocket() || stun_reflected_addr != socket_->GetLocalAddress()) &&
      !HasCandidateWithAddress(stun_reflected_addr)) {
    SocketAddress related_address = socket_->GetLocalAddress();
    // If we can't stamp the related address correctly, empty it to avoid leak.
    if (!MaybeSetDefaultLocalAddress(&related_address)) {
      related_address = EmptySocketAddressWithFamily(related_address.family());
    }

    std::stringstream url;
    url << "stun:" << stun_server_addr.ipaddr().ToString() << ":"
        << stun_server_addr.port();
    AddAddress(stun_reflected_addr, socket_->GetLocalAddress(), related_address,
               UDP_PROTOCOL_NAME, "", "", STUN_PORT_TYPE,
               ICE_TYPE_PREFERENCE_SRFLX, 0, url.str(), false);
  }
  MaybeSetPortCompleteOrError();
}

void UDPPort::OnStunBindingOrResolveRequestFailed(
    const SocketAddress& stun_server_addr, int error_code,
    const std::string& reason) {
  std::stringstream url;
  url << "stun:" << stun_server_addr.ToString();
  SignalCandidateError(
      this, IceCandidateErrorEvent(GetLocalAddress().HostAsSensitiveURIString(),
                                   GetLocalAddress().port(), url.str(),
                                   error_code, reason));
  if (bind_request_failed_servers_.find(stun_server_addr) !=
      bind_request_failed_servers_.end()) {
    return;
  }
  bind_request_failed_servers_.insert(stun_server_addr);
  MaybeSetPortCompleteOrError();
}

void UDPPort::MaybeSetPortCompleteOrError() {
  if (mdns_name_registration_status() ==
      MdnsNameRegistrationStatus::kInProgress) {
    return;
  }

  if (ready_) {
    return;
  }

  // Do not set port ready if we are still waiting for bind responses.
  const size_t servers_done_bind_request =
      bind_request_failed_servers_.size() +
      bind_request_succeeded_servers_.size();
  if (server_addresses_.size() != servers_done_bind_request) {
    return;
  }

  // Setting ready status.
  ready_ = true;

  // The port is "completed" if there is no stun server provided, or the bind
  // request succeeded for any stun server, or the socket is shared.
  if (server_addresses_.empty() || bind_request_succeeded_servers_.size() > 0 ||
      SharedSocket()) {
    SignalPortComplete(this);
  } else {
    SignalPortError(this);
  }
}

// TODO(?): merge this with SendTo above.
void UDPPort::OnSendPacket(const void* data, size_t size, StunRequest* req) {
  StunBindingRequest* sreq = static_cast<StunBindingRequest*>(req);
  PacketOptions options(StunDscpValue());
  options.info_signaled_after_sent.packet_type = PacketType::kStunMessage;
  CopyPortInformationToPacketInfo(&options.info_signaled_after_sent);
  if (socket_->SendTo(data, size, sreq->server_addr(), options) < 0) {
    LOG(ERROR) << socket_->GetError() << " sendto";
  }
  stats_.stun_binding_requests_sent++;
}

bool UDPPort::HasCandidateWithAddress(const SocketAddress& addr) const {
  const std::vector<Candidate>& existing_candidates = Candidates();
  std::vector<Candidate>::const_iterator it = existing_candidates.begin();
  for (; it != existing_candidates.end(); ++it) {
    if (it->address() == addr) return true;
  }
  return false;
}

std::unique_ptr<StunPort> StunPort::Create(
    PacketSocketFactory* factory, Network* network, uint16_t min_port,
    uint16_t max_port, const std::string& username, const std::string& password,
    const ServerAddresses& servers, const std::string& origin,
    std::optional<int> stun_keepalive_interval) {
  // Using `new` to access a non-public constructor.
  // auto port =
  //     std::make_unique<StunPort>(factory, network, min_port, max_port,
  //                                  username, password, servers, origin);
  // port->set_stun_keepalive_delay(stun_keepalive_interval);
  // if (!port->Init()) {
  //   return nullptr;
  // }
  // return port;
  return nullptr;
}

StunPort::StunPort(PacketSocketFactory* factory, Network* network,
                   uint16_t min_port, uint16_t max_port,
                   const std::string& username, const std::string& password,
                   const ServerAddresses& servers, const std::string& origin)
    : UDPPort(factory, network, min_port, max_port, username, password, origin,
              false) {
  // UDPPort will set these to local udp, updating these to STUN.
  set_type(STUN_PORT_TYPE);
  set_server_addresses(servers);
}

void StunPort::PrepareAddress() { SendStunBindingRequests(); }

}  // namespace cricket
