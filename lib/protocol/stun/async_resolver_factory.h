#ifndef API_ASYNC_RESOLVER_FACTORY_H_
#define API_ASYNC_RESOLVER_FACTORY_H_

#include "async_resolver_interface.h"

namespace MSF {

// An abstract factory for creating AsyncResolverInterfaces. This allows
// client applications to provide WebRTC with their own mechanism for
// performing DNS resolution.
class AsyncResolverFactory {
 public:
  AsyncResolverFactory() = default;
  virtual ~AsyncResolverFactory() = default;

  // The caller should call Destroy on the returned object to delete it.
  virtual AsyncResolverInterface* Create() = 0;
};

}  // namespace MSF

#endif  // API_ASYNC_RESOLVER_FACTORY_H_
