#ifndef RTC_BASE_NET_HELPER_H_
#define RTC_BASE_NET_HELPER_H_

#include <string>

// This header contains helper functions and constants used by different types
// of transports.
namespace MSF {

extern const char UDP_PROTOCOL_NAME[];
extern const char TCP_PROTOCOL_NAME[];
extern const char SSLTCP_PROTOCOL_NAME[];
extern const char TLS_PROTOCOL_NAME[];

// Get the network layer overhead per packet based on the IP address family.
int GetIpOverhead(int addr_family);

// Get the transport layer overhead per packet based on the protocol.
int GetProtocolOverhead(const std::string& protocol);

}  // namespace MSF

#endif  // RTC_BASE_NET_HELPER_H_
