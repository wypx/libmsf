#include "turn_port_factory.h"

#include <memory>
#include <utility>

#include "port_allocator.h"
#include "turn_port.h"

namespace MSF {

TurnPortFactory::~TurnPortFactory() {}

std::unique_ptr<Port> TurnPortFactory::Create(const CreateRelayPortArgs& args,
                                              AsyncPacketSocket* udp_socket) {
  auto port = TurnPort::CreateUnique(
      args.loop, args.socket_factory, args.network, udp_socket, args.username,
      args.password, *args.server_address, args.config->credentials,
      args.config->priority, args.origin, args.turn_customizer);
  port->SetTlsCertPolicy(args.config->tls_cert_policy);
  port->SetTurnLoggingId(args.config->turn_logging_id);
  return std::move(port);
}

#if 0
std::unique_ptr<Port> TurnPortFactory::Create(const CreateRelayPortArgs& args,
                                              int min_port, int max_port) {
  auto port = TurnPort::CreateUnique(
      args.loop, args.socket_factory, args.network, min_port,
      max_port, args.username, args.password, *args.server_address,
      args.config->credentials, args.config->priority, args.origin,
      args.config->tls_alpn_protocols, args.config->tls_elliptic_curves,
      args.turn_customizer, args.config->tls_cert_verifier);
  port->SetTlsCertPolicy(args.config->tls_cert_policy);
  port->SetTurnLoggingId(args.config->turn_logging_id);
  return std::move(port);
}
#endif

}  // namespace cricket
