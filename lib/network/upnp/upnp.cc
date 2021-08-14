/**************************************************************************
 *
 * Copyright (c) 2017-2021, luotang.me <wypx520@gmail.com>, China.
 * All rights reserved.
 *
 * Distributed under the terms of the GNU General Public License v2.
 *
 * This software is provided 'as is' with no explicit or implied warranties
 * in respect of its properties, including, but not limited to, correctness
 * and/or fitness for purpose.
 *
 **************************************************************************/
#include "upnp.h"

#include <base/logging.h>

namespace MSF {

UPnPManager::UPnPManager() : name_("luotang.me") {}

UPnPManager::~UPnPManager() {
  if (discovered_) ::FreeUPNPUrls(&urls_);
}

struct UPNPDev *UPnPManager::Discover(int msec) {
  struct UPNPDev *ret;
  bool have_err;

#if (MINIUPNPC_API_VERSION >= 8) /* adds ipv6 and error args */
  int err = UPNPDISCOVER_SUCCESS;
#if (MINIUPNPC_API_VERSION >= 14) /* adds ttl */
  ret = ::upnpDiscover(msec, nullptr, nullptr, 0, 0, 2, &err);
#else
  ret = ::upnpDiscover(msec, nullptr, nullptr, 0, 0, &err);
#endif
  have_err = (err != UPNPDISCOVER_SUCCESS);
#else
  ret = ::upnpDiscover(msec, nullptr, nullptr, 0);
  have_err = (ret == nullptr);
#endif

  if (have_err) {
    LOG(ERROR) << "upnp discover errno: " << errno;
  }
  return ret;
}

bool UPnPManager::GetSpecificPortMappingEntry(const char *proto,
                                              uint16_t port) {
  int err;
  const int old_errno = errno;
  char int_client[16] = {0};
  char int_port[16] = {0};

  std::string port_str = std::to_string(port);

#if (MINIUPNPC_API_VERSION >= 10)
  /* adds remoteHost arg */
  err = ::UPNP_GetSpecificPortMappingEntry(
      urls_.controlURL, data_.first.servicetype, port_str.c_str(), proto,
      nullptr, int_client, int_port, nullptr, nullptr, nullptr);
#elif (MINIUPNPC_API_VERSION > 8)
  /* adds desc, enabled and leaseDuration args */
  err = ::UPNP_GetSpecificPortMappingEntry(
      urls_.controlURL, data_.first.servicetype, port_str.c_str(), proto,
      int_client, int_port, nullptr, nullptr, nullptr);
#else
  err = ::UPNP_GetSpecificPortMappingEntry(
      urls_.controlURL, data_.first.servicetype, port_str.c_str(), proto,
      int_client, int_port);

#endif
  if (err) {
    LOG(ERROR) << "get " << proto << " port forwarding error: " << err
               << " errno: " << errno;
    errno = old_errno;
  }
  return (err == UPNPCOMMAND_SUCCESS);
}

bool UPnPManager::AddPortMapping(const char *proto, uint16_t port,
                                 const char *desc) {
  int err;
  const int old_errno = errno;
  std::string port_str = std::to_string(port);
  errno = 0;

#if (MINIUPNPC_API_VERSION >= 8)
  err = ::UPNP_AddPortMapping(urls_.controlURL, data_.first.servicetype,
                              port_str.c_str(), port_str.c_str(), lanaddr_,
                              desc, proto, nullptr, nullptr);
#else
  err = ::UPNP_AddPortMapping(urls_.controlURL, data_.first.servicetype,
                              port_str.c_str(), port_str.c_str(), lanaddr_,
                              desc, proto, nullptr);
#endif

  if (err) {
    LOG(ERROR) << "add " << proto << " port forwarding error: " << err
               << " errno: " << errno;
    errno = old_errno;
  }

  return (err == UPNPCOMMAND_SUCCESS);
}

bool UPnPManager::DeletePortMapping(const char *proto, uint16_t port) {
  const int old_errno = errno;
  std::string port_str = std::to_string(port);

  int err = ::UPNP_DeletePortMapping(urls_.controlURL, data_.first.servicetype,
                                     port_str.c_str(), proto, nullptr);

  if (err) {
    LOG(ERROR) << "del " << proto << " port forwarding error: " << err
               << " errno: " << errno;
    errno = old_errno;
  }
  return (err == UPNPCOMMAND_SUCCESS);
}

enum {
  UPNP_IGD_NONE = 0,
  UPNP_IGD_VALID_CONNECTED = 1,
  UPNP_IGD_VALID_NOT_CONNECTED = 2,
  UPNP_IGD_INVALID = 3
};


bool UPnPManager::Pluse(uint16_t port, bool enable, bool port_check) {
  UPnPMapKey tcp_key = std::make_pair("TCP", port);
  UPnPMapKey udp_key = std::make_pair("UDP", port);

  UPnPMapEntryPtr tcp_entry;
  UPnPMapEntryPtr udp_entry;
  auto it = map_entry_.find(tcp_key);
  if (it == map_entry_.end()) {
    UPnPMapEntryPtr entry(new UPnPMapEntry("TCP", port, enable));
    tcp_entry = entry;
    map_entry_[tcp_key] = entry;
    // map_entry_.insert({tcp_key, entry});
  } else {
    tcp_entry = it->second;
  }
  it = map_entry_.find(udp_key);
  if (it == map_entry_.end()) {
    UPnPMapEntryPtr entry(new UPnPMapEntry("UDP", port, enable));
    udp_entry = entry;
    map_entry_[udp_key] = entry;
  } else {
    udp_entry = it->second;
  }

  int ret;
  if (!discovered_) {
    ::memset(lanaddr_, 0, sizeof(lanaddr_));
    struct UPNPDev *devlist = Discover();
    ret =
        ::UPNP_GetValidIGD(devlist, &urls_, &data_, lanaddr_, sizeof(lanaddr_));
    if (ret == UPNP_IGD_VALID_CONNECTED) {
      LOG(INFO) << "found internet gateway device: " << urls_.controlURL
                << " local address: " << lanaddr_;
      discovered_ = true;
    } else {
      LOG(ERROR) << "upnp get valid igd errno: " << errno
                 << ", please make sure router supports upnp and enabled";
      return false;
    }
  }

  bool success = false;
  // Check port mapping and do mapping
  if (GetSpecificPortMappingEntry("TCP", port)) {
    if (enable) {
      LOG(INFO) << "port " << port << " has forwarded";
    } else {
      std::string desc = name_ + " at " + std::to_string(port);

      success = AddPortMapping("TCP", port, desc.c_str());
      tcp_entry->set_finished(success);
      LOG(INFO) << "port forwarding through " << urls_.controlURL << " service "
                << data_.first.servicetype << " (local address: " << lanaddr_
                << ":" << port << ")";
    }
  } else {
    if (!enable) {
      LOG(INFO) << "port " << port << " isn't forwarded";
    } else {
      success = DeletePortMapping("TCP", port);
      tcp_entry->set_finished(success);
      LOG(INFO) << "stopping port forwarding through " << urls_.controlURL
                << ", service " << data_.first.servicetype;
    }
  }

  if (GetSpecificPortMappingEntry("UDP", port)) {
    if (enable) {
      LOG(INFO) << "port " << port << " has forwarded";
    } else {
      std::string desc = name_ + " at " + std::to_string(port);

      success = AddPortMapping("UDP", port, desc.c_str());
      udp_entry->set_finished(success);
      LOG(INFO) << "port forwarding through " << urls_.controlURL << " service "
                << data_.first.servicetype << " (local address: " << lanaddr_
                << ":" << port << ")";
    }
  } else {
    if (!enable) {
      LOG(INFO) << "port " << port << " isn't forwarded";
    } else {
      success = DeletePortMapping("UDP", port);
      udp_entry->set_finished(success);
      LOG(INFO) << "stopping port forwarding through " << urls_.controlURL
                << ", service " << data_.first.servicetype;
    }
  }
  return success;
}

}  // namespace MSF
