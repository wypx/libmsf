
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
#ifndef BASE_UPNP_H_
#define BASE_UPNP_H_

#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#include <sock/inet_address.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace MSF {

struct UPnPMapEntry {
  UPnPMapEntry(const std::string &proto, uint16_t port, bool enable)
      : proto_(proto), port_(port), enable_(enable) {}

  const std::string &proto() const { return proto_; }
  // port changed handled by user buessiness
  uint16_t port() const { return port_; }
  bool enable() const { return enable_; }

  void set_finished(bool finished) noexcept { finished_ = finished; }
  bool finished() const { return finished_; }

  std::string proto_;
  uint16_t port_ = 0;
  bool enable_ = false;
  bool finished_ = false;
};

typedef std::shared_ptr<UPnPMapEntry> UPnPMapEntryPtr;

typedef std::pair<std::string, uint16_t> UPnPMapKey;
struct UPnPMapHash {
  std::size_t operator()(UPnPMapKey const &item) const {
    std::size_t h1 = std::hash<std::string>()(item.first);
    std::size_t h2 = std::hash<uint16_t>()(item.second);
    return h1 ^ (h2 << 1);
  }
};

class UPnPManager {
 public:
  UPnPManager();
  ~UPnPManager();

  void set_discovered(bool discovered) noexcept { discovered_ = discovered; }
  bool discovered() const { return discovered_; }

  bool Pluse(uint16_t port, bool enable, bool port_check);

 private:
  struct UPNPDev *Discover(int msec = 2000);
  bool GetSpecificPortMappingEntry(const char *proto, uint16_t port);
  bool AddPortMapping(const char *proto, uint16_t port, const char *desc);
  bool DeletePortMapping(const char *proto, uint16_t port);

 private:
  /**
   * Must set discovered false
   * First when upnp init,
   * Then lan network changed
   **/
  bool discovered_ = false;
  struct UPNPUrls urls_;
  struct IGDdatas data_;
  char lanaddr_[16];
  std::string name_;

  std::unordered_map<UPnPMapKey, UPnPMapEntryPtr, UPnPMapHash> map_entry_;
};

}  // namespace MSF
#endif