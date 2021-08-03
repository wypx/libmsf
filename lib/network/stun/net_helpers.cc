
#include <ifaddrs.h>

#include "byte_order.h"
#include "net_helpers.h"

namespace MSF {

const char* inet_ntop(int af, const void* src, char* dst, socklen_t size) {
  return ::inet_ntop(af, src, dst, size);
}

int inet_pton(int af, const char* src, void* dst) {
  return ::inet_pton(af, src, dst);
}

bool HasIPv4Enabled() {
#if defined(__linux__)
  bool has_ipv4 = false;
  struct ifaddrs* ifa;
  if (getifaddrs(&ifa) < 0) {
    return false;
  }
  for (struct ifaddrs* cur = ifa; cur != nullptr; cur = cur->ifa_next) {
    if (cur->ifa_addr->sa_family == AF_INET) {
      has_ipv4 = true;
      break;
    }
  }
  freeifaddrs(ifa);
  return has_ipv4;
#else
  return true;
#endif
}

#if defined(WIN32) || defined(WIN64)
enum WindowsMajorVersions {
  kWindows2000 = 5,
  kWindowsVista = 6,
  kWindows10 = 10,
};
bool GetOsVersion(int* major, int* minor, int* build) {
  OSVERSIONINFO info = {0};
  info.dwOSVersionInfoSize = sizeof(info);
  if (::GetVersionEx(&info)) {
    if (major) *major = info.dwMajorVersion;
    if (minor) *minor = info.dwMinorVersion;
    if (build) *build = info.dwBuildNumber;
    return true;
  }
  return false;
}

inline bool IsWindowsVistaOrLater() {
  int major;
  return (GetOsVersion(&major, nullptr, nullptr) && major >= kWindowsVista);
}

inline bool IsWindowsXpOrLater() {
  int major, minor;
  return (GetOsVersion(&major, &minor, nullptr) &&
          (major >= kWindowsVista || (major == kWindows2000 && minor >= 1)));
}

inline bool IsWindows8OrLater() {
  int major, minor;
  return (GetOsVersion(&major, &minor, nullptr) &&
          (major > kWindowsVista || (major == kWindowsVista && minor >= 2)));
}
#endif

bool HasIPv6Enabled() {
#if defined(WINUWP)
  // WinUWP always has IPv6 capability.
  return true;
#elif defined(WIN32) || defined(WIN64)
  if (IsWindowsVistaOrLater()) {
    return true;
  }
  if (!IsWindowsXpOrLater()) {
    return false;
  }
  DWORD protbuff_size = 4096;
  std::unique_ptr<char[]> protocols;
  LPWSAPROTOCOL_INFOW protocol_infos = nullptr;
  int requested_protocols[2] = {AF_INET6, 0};

  int err = 0;
  int ret = 0;
  // Check for protocols in a do-while loop until we provide a buffer large
  // enough. (WSCEnumProtocols sets protbuff_size to its desired value).
  // It is extremely unlikely that this will loop more than once.
  do {
    protocols.reset(new char[protbuff_size]);
    protocol_infos = reinterpret_cast<LPWSAPROTOCOL_INFOW>(protocols.get());
    ret = WSCEnumProtocols(requested_protocols, protocol_infos, &protbuff_size,
                           &err);
  } while (ret == SOCKET_ERROR && err == WSAENOBUFS);

  if (ret == SOCKET_ERROR) {
    return false;
  }

  // Even if ret is positive, check specifically for IPv6.
  // Non-IPv6 enabled WinXP will still return a RAW protocol.
  for (int i = 0; i < ret; ++i) {
    if (protocol_infos[i].iAddressFamily == AF_INET6) {
      return true;
    }
  }
  return false;
#elif defined(__linux__)
  bool has_ipv6 = false;
  struct ifaddrs* ifa;
  if (getifaddrs(&ifa) < 0) {
    return false;
  }
  for (struct ifaddrs* cur = ifa; cur != nullptr; cur = cur->ifa_next) {
    if (cur->ifa_addr->sa_family == AF_INET6) {
      has_ipv6 = true;
      break;
    }
  }
  freeifaddrs(ifa);
  return has_ipv6;
#else
  return true;
#endif
}

}  // namespace MSF
