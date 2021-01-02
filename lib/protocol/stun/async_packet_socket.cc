#include "async_packet_socket.h"
#include "net_helper.h"

namespace MSF {

PacketTimeUpdateParams::PacketTimeUpdateParams() = default;

PacketTimeUpdateParams::PacketTimeUpdateParams(
    const PacketTimeUpdateParams& other) = default;

PacketTimeUpdateParams::~PacketTimeUpdateParams() = default;

PacketOptions::PacketOptions() = default;
PacketOptions::PacketOptions(DiffServCodePoint dscp) : dscp(dscp) {}
PacketOptions::PacketOptions(const PacketOptions& other) = default;
PacketOptions::~PacketOptions() = default;

AsyncPacketSocket::AsyncPacketSocket() = default;

AsyncPacketSocket::~AsyncPacketSocket() = default;

void CopySocketInformationToPacketInfo(size_t packet_size_bytes,
                                       const AsyncPacketSocket& socket_from,
                                       bool is_connectionless,
                                       MSF::PacketInfo* info) {
  info->packet_size_bytes = packet_size_bytes;
  // TODO(srte): Make sure that the family of the local socket is always set
  // in the VirtualSocket implementation and remove this check.
  int family = socket_from.GetLocalAddress().family();
  if (family != 0) {
    info->ip_overhead_bytes = GetIpOverhead(family);
  }
}

}  // namespace MSF
