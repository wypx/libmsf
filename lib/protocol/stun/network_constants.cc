#include "network_constants.h"
#include <butil/logging.h>

namespace MSF {

std::string AdapterTypeToString(AdapterType type) {
  switch (type) {
    case ADAPTER_TYPE_ANY:
      return "Wildcard";
    case ADAPTER_TYPE_UNKNOWN:
      return "Unknown";
    case ADAPTER_TYPE_ETHERNET:
      return "Ethernet";
    case ADAPTER_TYPE_WIFI:
      return "Wifi";
    case ADAPTER_TYPE_CELLULAR:
      return "Cellular";
    case ADAPTER_TYPE_CELLULAR_2G:
      return "Cellular2G";
    case ADAPTER_TYPE_CELLULAR_3G:
      return "Cellular3G";
    case ADAPTER_TYPE_CELLULAR_4G:
      return "Cellular4G";
    case ADAPTER_TYPE_CELLULAR_5G:
      return "Cellular5G";
    case ADAPTER_TYPE_VPN:
      return "VPN";
    case ADAPTER_TYPE_LOOPBACK:
      return "Loopback";
    default:
      LOG(ERROR) << "Invalid type " << type;
      return std::string();
  }
}

}  // namespace MSF
