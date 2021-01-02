#ifndef RTC_BASE_ASYNC_RESOLVER_INTERFACE_H_
#define RTC_BASE_ASYNC_RESOLVER_INTERFACE_H_

#include "socket_address.h"
#include "sigslot.h"

namespace MSF {

// This interface defines the methods to resolve the address asynchronously.
class AsyncResolverInterface {
 public:
  AsyncResolverInterface();
  virtual ~AsyncResolverInterface();

  // Start address resolution of the hostname in |addr|.
  virtual void Start(const SocketAddress& addr) = 0;
  // Returns true iff the address from |Start| was successfully resolved.
  // If the address was successfully resolved, sets |addr| to a copy of the
  // address from |Start| with the IP address set to the top most resolved
  // address of |family| (|addr| will have both hostname and the resolved ip).
  virtual bool GetResolvedAddress(int family, SocketAddress* addr) const = 0;
  // Returns error from resolver.
  virtual int GetError() const = 0;
  // Delete the resolver.
  virtual void Destroy(bool wait) = 0;
  // Returns top most resolved IPv4 address if address is resolved successfully.
  // Otherwise returns address set in SetAddress.
  SocketAddress address() const {
    SocketAddress addr;
    GetResolvedAddress(AF_INET, &addr);
    return addr;
  }

  // This signal is fired when address resolve process is completed.
  signal1<AsyncResolverInterface*> SignalDone;
};

}  // namespace MSF

#endif
