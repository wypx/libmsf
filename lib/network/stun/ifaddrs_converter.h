#ifndef RTC_BASE_IFADDRS_CONVERTER_H_
#define RTC_BASE_IFADDRS_CONVERTER_H_

#if defined(__linux__)
#include <ifaddrs.h>
#endif

#include "ip_address.h"

namespace MSF {

// This class converts native interface addresses to our internal IPAddress
// class. Subclasses should override ConvertNativeToIPAttributes to implement
// the different ways of retrieving IPv6 attributes for various POSIX platforms.
class IfAddrsConverter {
 public:
  IfAddrsConverter();
  virtual ~IfAddrsConverter();
  virtual bool ConvertIfAddrsToIPAddress(const struct ifaddrs* interface,
                                         InterfaceAddress* ipaddress,
                                         IPAddress* mask);

 protected:
  virtual bool ConvertNativeAttributesToIPAttributes(
      const struct ifaddrs* interface, int* ip_attributes);
};

IfAddrsConverter* CreateIfAddrsConverter();

}  // namespace MSF

#endif  // RTC_BASE_IFADDRS_CONVERTER_H_
