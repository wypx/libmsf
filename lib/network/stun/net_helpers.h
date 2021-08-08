#ifndef RTC_BASE_NET_HELPERS_H_
#define RTC_BASE_NET_HELPERS_H_

#include <list>

#include <netdb.h>
#include <stddef.h>

namespace MSF {

// rtc namespaced wrappers for inet_ntop and inet_pton so we can avoid
// the windows-native versions of these.
const char* inet_ntop(int af, const void* src, char* dst, socklen_t size);
int inet_pton(int af, const char* src, void* dst);

bool HasIPv4Enabled();
bool HasIPv6Enabled();

}  // namespace MSF

#endif  //__RTCBASE_NET_HELPERS_H_
