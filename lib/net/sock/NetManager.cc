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
#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#define __KERNEL__
#include <linux/ethtool.h>
#undef __KERNEL__

#include <butil/logging.h>

#include "NetManager.h"
#include "Utils.h"

namespace MSF {

#define DEFAULT_RESOLV_FILE "/etc/resolv.conf"
#define DEFAULT_ROUTE_FILE "/proc/net/route"
#define DEFAULT_IF_INET6_FILE "/proc/net/if_inet6"
#define DEFAULT_IPV6_ROUTE_FILE "/proc/net/ipv6_route"

struct in6_ifreq {
  struct in6_addr ifr6_addr;
  unsigned int ifr6_prefixlen;
  unsigned int ifr6_ifindex;
};

static uint32_t g_old_gateway = 0;
const struct in6_addr g_any6addr = IN6ADDR_ANY_INIT;
/*
 * Set Net Config
 * 1.Use system() or exec*() to call ifconfig and route.
 *   Advantage:	Simple
 *   Disadvantage: ineffective, but depend on ifconfig or route command.
 * 2.Create Socket,use iocrl().
 *   Advantage:	effective
 *   Disadvantage: diffcult.
 *
 *  Get Net Config
 *  1.Use popen(), other side execute ifconfig and route,
 *    this side recv and parse.
 *  2.Use fopen() to open /proc/net/route, get gateway.
 *    effective than ifconfig and route command.
 *  3. Create Socket,use iocrl().
 */

/**
 * Linux下查看网关方法:
 * 1. ifconfig -a
 * 2. route -n
 * 3. ip route show
 * 4. traceroute www.prudentwoo.com -s 100 第一行就是自己的默认网关
 * 5. netstat -rn
 * 6. more /etc/network/interfaces Debian/Ubuntu Linux
 * 7. more /etc/sysconfig/network-scripts/ifcfg-eth0 Red Hat
 *
 * 代码实现:
 * https://blog.csdn.net/liangyamin/article/details/7242048
 * */
std::string GetGateway(const std::string &iface) {
  std::string gateway = "";

  int ret = -1;
  FILE *fp = nullptr;
  struct in_addr mask, dst, gw;
  uint64_t d, g, m = 0;
  int flags, ref, use, metric, mtu, win, ir = 0;
  char devname[64];

  memset(devname, 0, sizeof(devname));
  memset(&mask, 0, sizeof(mask));
  memset(&dst, 0, sizeof(dst));
  memset(&gw, 0, sizeof(gw));

  fp = fopen(DEFAULT_ROUTE_FILE, "r");
  if (NULL == fp) {
    LOG(ERROR) << "open route file failed: " << errno;
    return gateway;
  }

  if (fscanf(fp, "%*[^\n]\n") < 0) {
    /* Skip the first line.
     * Empty or missing line, or read error. */
    fclose(fp);
    return gateway;
  }

  while (true) {
    ret = fscanf(fp, "%63s%lx%lx%X%d%d%d%lx%d%d%d\n", devname, &d, &g, &flags,
                 &ref, &use, &metric, &m, &mtu, &win, &ir);
    if (ret != 11) {
      LOG(ERROR) << "fail to scanf route file, item not match";
      break;
    }
    if (!(flags & RTF_UP)) { /* Skip interfaces that are down. */
      continue;
    }
    if (strcmp(devname, iface.c_str()) != 0) {
      continue;
    }
    mask.s_addr = m;
    dst.s_addr = d;
    gw.s_addr = g;

    memset(devname, 0, sizeof(devname));
    if (flags & RTF_GATEWAY) {
      inet_ntop(AF_INET, &gw, devname, sizeof(devname));
      gateway = devname;
      break;
    }
  }
  fclose(fp);
  return gateway;
}

std::string GetNetmask(const std::string &iface) {
  std::string subnet = "";

  int fd = -1;
  struct ifreq ifrmask;
  struct sockaddr_in *netmaskaddr;

  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    LOG(ERROR) << "create socket fail, errno: " << errno;
    return subnet;
  }
  memset(&ifrmask, 0, sizeof(ifrmask));
  strlcpy(ifrmask.ifr_name, iface.c_str(), sizeof(ifrmask.ifr_name) - 1);

  if ((ioctl(fd, SIOCGIFNETMASK, &ifrmask)) < 0) {
    LOG(ERROR) << "ioctrl get netmask failed";
    close(fd);
    return subnet;
  }

  netmaskaddr = (struct sockaddr_in *)&(ifrmask.ifr_ifru.ifru_netmask);
  inet_pton(AF_INET, subnet.c_str(), &netmaskaddr->sin_addr);

  LOG(ERROR) << "current netmask:" << subnet;

  close(fd);
  return subnet;
}

bool SetNetmask(const std::string &iface, const std::string &netmask) {
  int fd = -1;
  struct ifreq ifr;
  struct sockaddr_in netmaskaddr;

  memset(&ifr, 0, sizeof(ifr));
  memset(&netmaskaddr, 0, sizeof(netmaskaddr));

  if (netmask.empty() || iface.empty()) {
    return false;
  }
  if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    return -1;
  }

  strlcpy(ifr.ifr_name, iface.c_str(), sizeof(ifr.ifr_name) - 1);
  bzero(&netmaskaddr, sizeof(struct sockaddr_in));
  netmaskaddr.sin_family = PF_INET;
  (void)inet_aton(netmask.c_str(), &netmaskaddr.sin_addr);

  memcpy(&ifr.ifr_ifru.ifru_netmask, &netmaskaddr, sizeof(struct sockaddr_in));
  if (ioctl(fd, SIOCSIFNETMASK, &ifr) < 0) {
    LOG(ERROR) << "set_netmask ioctl error and errno:" << errno;
    close(fd);
    return false;
  }
  close(fd);
  return true;
}

std::string GetMacAddr(const std::string &iface) {
  std::string macaddr = "";

  int fd = -1;
  struct ifreq ifr = {0};
  uint8_t *ptr = nullptr;
  char macstr[8];
  // if (0 == strcmp(iface.c_str(), "lo")) {
  //   return macaddr;
  // }
  if (iface == "lo") {
    return macaddr;
  }

  if ((fd = ::socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    LOG(ERROR) << "create socket failed, errno :" << errno;
    return macaddr;
  }

  strlcpy(ifr.ifr_name, iface.c_str(), sizeof(ifr.ifr_name));

  if (::ioctl(fd, SIOCGIFHWADDR, &ifr) != 0) {
    LOG(ERROR) << "get_mac ioctl error and errno: " << errno;
    close(fd);
    return macaddr;
  }

  ptr = (uint8_t *)&ifr.ifr_ifru.ifru_hwaddr.sa_data[0];
  snprintf(macstr, 6, "%02x-%02x-%02x-%02x-%02x-%02x", *ptr, *(ptr + 1),
           *(ptr + 2), *(ptr + 3), *(ptr + 4), *(ptr + 5));
  ::close(fd);
  macaddr = macstr;
  return macaddr;
}

bool SetMacAddr(const std::string &iface, const std::string &macstr) {
  int fd = -1;
  struct ifreq ifr;
  sa_family_t get_family = 0;
  short tmp = 0;
  int i = 0;
  int j = 0;

  memset(&ifr, 0, sizeof(ifr));

  if (macstr.empty() || iface.empty()) {
    return false;
  }

  if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    return false;
  }

  strlcpy(ifr.ifr_name, iface.c_str(), sizeof(ifr.ifr_name));

  if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
    LOG(ERROR) << "ioctl error and errno: " << errno;
    close(fd);
    return false;
  }

  get_family = ifr.ifr_ifru.ifru_hwaddr.sa_family;

  if (!ifaceDown(iface)) {
    return false;
  }

  bzero(&ifr, sizeof(struct ifreq));
  strlcpy(ifr.ifr_name, iface.c_str(), sizeof(ifr.ifr_name));
  ifr.ifr_ifru.ifru_hwaddr.sa_family = get_family;

  j = 0;
  for (i = 0; i < 17; i += 3) {
    if (macstr[i] < 58 && macstr[i] > 47) {
      tmp = macstr[i] - 48;
    }
    if (macstr[i] < 71 && macstr[i] > 64) {
      tmp = macstr[i] - 55;
    }
    if (macstr[i] < 103 && macstr[i] > 96) {
      tmp = macstr[i] - 87;
    }

    tmp = tmp << 4;
    if (macstr[i + 1] < 58 && macstr[i + 1] > 47) {
      tmp |= (macstr[i + 1] - 48);
    }
    if (macstr[i + 1] < 71 && macstr[i + 1] > 64) {
      tmp |= (macstr[i + 1] - 55);
    }
    if (macstr[i + 1] < 103 && macstr[i + 1] > 96) {
      tmp |= (macstr[i + 1] - 87);
    }
    memcpy(&ifr.ifr_ifru.ifru_hwaddr.sa_data[j++], &tmp, 1);
  }

  if (ioctl(fd, SIOCSIFHWADDR, &ifr) < 0) {
    LOG(ERROR) << "set_mac ioctl error and errno: " << errno;
    close(fd);
    return false;
  }

  if (!ifaceUp(iface)) {
    close(fd);
    return false;
  }
  close(fd);
  return true;
}

uint32_t GetMacAddrNum() {
  uint32_t count = 0;
  FILE *f = ::fopen("/proc/net/dev", "r");
  if (!f) {
    LOG(ERROR) << "open /proc/net/dev errno:" << errno;
    return count;
  }

  char szLine[512];
  char szName[128];
  int nLen;

  /* eat line */
  if (!::fgets(szLine, sizeof(szLine), f)) {
    return count;
  }
  if (!::fgets(szLine, sizeof(szLine), f)) {
    return count;
  }

  while (::fgets(szLine, sizeof(szLine), f)) {
    memset(szName, 0, sizeof(szName));
    sscanf(szLine, "%s", szName);
    nLen = strlen(szName);
    if (nLen <= 0) continue;
    if (szName[nLen - 1] == ':') {
      szName[nLen - 1] = 0;
    }
    if (strcmp(szName, "lo") == 0) continue;
    count++;
  }

  fclose(f);
  return count;
}

std::string GetBroadcastAddr(const std::string &iface) {
  std::string broadcast = "";

  int fd = -1;
  struct ifreq ifr;
  struct sockaddr_in *ptr = NULL;
  struct in_addr addr_temp;

  memset(&ifr, 0, sizeof(ifr));
  memset(&addr_temp, 0, sizeof(addr_temp));

  if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    LOG(ERROR) << "socket failed, errno : " << errno;
    return broadcast;
  }

  strlcpy(ifr.ifr_name, iface.c_str(), sizeof(ifr.ifr_name));

  if (ioctl(fd, SIOCGIFBRDADDR, &ifr) < 0) {
    LOG(ERROR) << "get_broadcast ioctl error and errno: " << errno;
    close(fd);
    return broadcast;
  }

  ptr = (struct sockaddr_in *)&ifr.ifr_ifru.ifru_broadaddr;
  addr_temp = ptr->sin_addr;

  char buff[64] = {0};
  (void)inet_ntop(AF_INET, &addr_temp, buff, 64);
  broadcast = buff;

  close(fd);
  return broadcast;
}

bool SetBroadcastAddr(const std::string &iface, const std::string &broadcast) {
  int fd = -1;
  struct ifreq ifr;
  struct sockaddr_in broadcastaddr;

  memset(&ifr, 0, sizeof(ifr));
  memset(&broadcastaddr, 0, sizeof(broadcastaddr));

  if (broadcast.empty() || iface.empty()) {
    return false;
  }

  if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    return false;
  }

  strlcpy(ifr.ifr_name, iface.c_str(), sizeof(ifr.ifr_name));
  bzero(&broadcastaddr, sizeof(struct sockaddr_in));
  broadcastaddr.sin_family = PF_INET;
  (void)inet_aton(broadcast.c_str(), &broadcastaddr.sin_addr);

  memcpy(&ifr.ifr_ifru.ifru_broadaddr, &broadcastaddr,
         sizeof(struct sockaddr_in));

  if (ioctl(fd, SIOCSIFBRDADDR, &ifr) < 0) {
    close(fd);
    LOG(ERROR) << "set_broadcast_addr ioctl error and errno:" << errno;
    return false;
  }

  close(fd);
  return true;
}

bool addIPv6(const std::string &ipv6, const uint32_t prefix) {
  struct in6_ifreq ifr6;
  struct ifreq ifr;
  int fd = -1;

  memset(&ifr6, 0, sizeof(ifr6));
  memset(&ifr, 0, sizeof(ifr));

  /*3~127*/
  if ((prefix < 3) || (prefix > 127) || ipv6.empty()) {
    return false;
  }

  if ((fd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
    return -1;
  }
  strlcpy(ifr.ifr_name, "eth0", sizeof(ifr.ifr_name));
  if (ioctl(fd, SIOCGIFINDEX, (char *)&ifr) < 0) {
    close(fd);
    return false;
  }

  inet_pton(AF_INET6, ipv6.c_str(), &ifr6.ifr6_addr);

  ifr6.ifr6_prefixlen = prefix;
  ifr6.ifr6_ifindex = ifr.ifr_ifindex;
  if (ioctl(fd, SIOCSIFADDR, &ifr6) < 0) {
    close(fd);
    return false;
  }

  close(fd);
  return true;
}

bool delIPv6(const std::string &ipv6, const uint32_t prefix) {
  struct ifreq ifr;
  struct in6_ifreq ifr6;
  int fd = -1;

  memset(&ifr, 0, sizeof(ifr));
  memset(&ifr6, 0, sizeof(ifr6));

  if ((fd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
    return false;
  }

  strlcpy(ifr.ifr_name, "eth0", sizeof(ifr.ifr_name));
  if (ioctl(fd, SIOCGIFINDEX, (caddr_t)&ifr) < 0) {
    close(fd);
    return false;
  }

  inet_pton(AF_INET6, ipv6.c_str(), &ifr6.ifr6_addr);

  ifr6.ifr6_ifindex = ifr.ifr_ifindex;
  ifr6.ifr6_prefixlen = prefix;
  if (ioctl(fd, SIOCDIFADDR, &ifr6) < 0) {
    if (errno != EADDRNOTAVAIL) {
      if (!(errno == EIO)) {
        LOG(ERROR) << "del_ipv6_addr: ioctl(SIOCDIFADDR).";
      }
    } else {
      LOG(ERROR)
          << "del_ipv6_addr: ioctl(SIOCDIFADDR): No such address, errno: "
          << errno;
    }
    close(fd);
    return false;
  }

  close(fd);
  return true;
}

bool setIPv6Gateway(const std::string &iface, const std::string &gateway) {
  int fd = -1;
  struct in6_addr any6addr = IN6ADDR_ANY_INIT;
  struct in6_rtmsg v6_rt;
  struct in6_addr valid_ipv6;
  struct ifreq ifr;

  memset(&v6_rt, 0, sizeof(v6_rt));
  memset(&valid_ipv6, 0, sizeof(valid_ipv6));
  memset(&ifr, 0, sizeof(ifr));

  if (gateway.empty() || iface.empty()) {
    return false;
  }

  memcpy(&valid_ipv6, &any6addr, sizeof(valid_ipv6));
  while (delIPv6Gateway(&valid_ipv6))
    ;

  if ((fd = socket(PF_INET6, SOCK_DGRAM, 0)) < 0) {
    return false;
  }

  memset(&v6_rt, 0, sizeof(struct in6_rtmsg));
  memcpy(&v6_rt.rtmsg_dst, &valid_ipv6, sizeof(struct in6_addr));
  v6_rt.rtmsg_flags = RTF_UP | RTF_GATEWAY;
  v6_rt.rtmsg_dst_len = 0;
  v6_rt.rtmsg_metric = 1;

  memset(&ifr, 0, sizeof(ifr));
  strlcpy(ifr.ifr_name, iface.c_str(), sizeof(ifr.ifr_name));
  (void)ioctl(fd, SIOGIFINDEX, &ifr);
  v6_rt.rtmsg_ifindex = ifr.ifr_ifindex;

  memcpy(&v6_rt.rtmsg_gateway, gateway.c_str(), sizeof(struct in6_addr));
  if (ioctl(fd, SIOCADDRT, &v6_rt) < 0) {
    close(fd);
    LOG(ERROR) << "add route ioctl error and errno=" << errno;
    return false;
  }
  close(fd);
  return true;
}

bool delIPv6Gateway(const struct in6_addr *ipaddr) {
  int fd = -1;
  struct in6_rtmsg v6_rt;
  struct sockaddr_in6 gateway_addr;

  memset(&v6_rt, 0, sizeof(v6_rt));
  memset(&gateway_addr, 0, sizeof(gateway_addr));

  if ((fd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
    return false;
  }
  memset(&v6_rt, 0, sizeof(v6_rt));
  memset(&gateway_addr, 0, sizeof(gateway_addr));
  memcpy(&gateway_addr.sin6_addr, &g_any6addr, sizeof(g_any6addr));
  gateway_addr.sin6_family = PF_INET6;
  memcpy(&v6_rt.rtmsg_gateway, &gateway_addr.sin6_addr,
         sizeof(struct in6_addr));
  memcpy(&v6_rt.rtmsg_dst, ipaddr, sizeof(struct in6_addr));
  v6_rt.rtmsg_flags &= ~RTF_UP;
  v6_rt.rtmsg_dst_len = 0;
  v6_rt.rtmsg_metric = 0;
  if (ioctl(fd, SIOCDELRT, &v6_rt) < 0) {
    close(fd);
    LOG(ERROR) << "del route ioctl error and errno: " << errno;
    return false;
  }
  close(fd);
  return true;
}

bool addIPv4(const std::string &iface, const std::string &ipv4) {
  int fd = -1;
  struct ifreq ifr;
  struct sockaddr_in addr;

  memset(&ifr, 0, sizeof(ifr));
  memset(&addr, 0, sizeof(addr));

  if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    return false;
  }

  strlcpy(ifr.ifr_name, iface.c_str(), sizeof(ifr.ifr_name));

  bzero(&addr, sizeof(struct sockaddr_in));
  addr.sin_family = PF_INET;
  (void)inet_aton(ipv4.c_str(), &addr.sin_addr);

  memcpy(&ifr.ifr_ifru.ifru_addr, &addr, sizeof(struct sockaddr_in));

  if (ioctl(fd, SIOCSIFADDR, &ifr) < 0) {
    LOG(ERROR) << "set_ipaddr ioctl error";
    close(fd);
    return false;
  }
  ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
  // get the status of the device
  if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
    perror("SIOCSIFFLAGS");
    return false;
  }

  close(fd);
  return true;
}

bool setIPv4Gateway(const std::string &iface, const std::string &gateway) {
  int fd = -1;
  struct rtentry rt;
  struct sockaddr_in gateway_addr;

  char gwip[16];
  struct in_addr gw;

  memset(&rt, 0, sizeof(rt));
  memset(&gateway_addr, 0, sizeof(gateway_addr));
  memset(gwip, 0, sizeof(gwip));
  memset(&gw, 0, sizeof(gw));

  if (!delIPv4Gateway()) {
    return false;
  }

  memset(gwip, 0, sizeof(gwip));
  if (g_old_gateway > 0) {
    memset(gwip, 0, sizeof(gwip));
    memset(&gw, 0, sizeof(struct in_addr));
    gw.s_addr = g_old_gateway;
    strncpy(gwip, inet_ntoa(gw), sizeof(gwip) - 1);
    (void)delActiveRoute(iface, gwip, "0.0.0.0");
  }

  if (0 == strcmp(gateway.c_str(), "0.0.0.0")) {
    LOG(ERROR) << "interface isn't set gateway.";
    return true;
  }

  // if(isSameSubnet(if_name, gateway) == 0)
  {
    /* set new gateway */
    if (!setActiveRoute(iface, gateway, "0.0.0.0", true)) {
      return false;
    }

    /* store new gateway */
    memset(&gw, 0, sizeof(struct in_addr));
    (void)inet_aton(gateway.c_str(), &gw);
    g_old_gateway = gw.s_addr;
  }

  if ((fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    return false;
  }

  memset((char *)&rt, 0, sizeof(struct rtentry));
  rt.rt_flags = RTF_UP | RTF_GATEWAY;

  bzero(&gateway_addr, sizeof(struct sockaddr_in));
  gateway_addr.sin_family = PF_INET;
  (void)inet_aton(gateway.c_str(), &gateway_addr.sin_addr);
  memcpy(&rt.rt_gateway, &gateway_addr, sizeof(struct sockaddr_in));

  bzero(&gateway_addr, sizeof(struct sockaddr_in));
  (void)inet_aton("0.0.0.0", &gateway_addr.sin_addr);
  gateway_addr.sin_family = PF_INET;
  memcpy(&rt.rt_genmask, &gateway_addr, sizeof(struct sockaddr_in));

  memcpy(&rt.rt_dst, &gateway_addr, sizeof(struct sockaddr_in));

  rt.rt_dev = (char *)iface.c_str();

  if (ioctl(fd, SIOCADDRT, &rt) < 0) {
    LOG(ERROR) << "set_gateway ioctl error and errno";
    close(fd);
    return false;
  }
  close(fd);
  return true;
}

bool delIPv4Gateway(void) {
  int fd = -1;
  struct rtentry rt;
  struct sockaddr_in gateway_addr;

  if ((fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    return false;
  }

  memset(&rt, 0, sizeof(struct rtentry));
  bzero(&gateway_addr, sizeof(struct sockaddr_in));
  (void)inet_aton("0.0.0.0", &gateway_addr.sin_addr);
  gateway_addr.sin_family = PF_INET;
  memcpy(&rt.rt_genmask, &gateway_addr, sizeof(struct sockaddr_in));
  memcpy(&rt.rt_dst, &gateway_addr, sizeof(struct sockaddr_in));

  if (ioctl(fd, SIOCDELRT, &rt) < 0) {
    LOG(ERROR) << "del_gateway ioctl error ";
    close(fd);
    if (ESRCH == errno || ENOENT == errno) {
      return true;  // linux general errno 3 :  No such process
    }
    return false;
  }
  close(fd);
  return true;
}

std::vector<std::string> getDnsList() {
  std::vector<std::string> dnslist;

  char buf[128];
  char dns[64];
  char name[64];
  FILE *fp = nullptr;

  memset(buf, 0, sizeof(buf));
  memset(name, 0, sizeof(name));
  memset(dns, 0, sizeof(dns));

  fp = fopen(DEFAULT_RESOLV_FILE, "r");
  if (NULL == fp) {
    LOG(ERROR) << "can not open file /etc/resolv.conf.";
    return dnslist;
  }
  while (fgets(buf, sizeof(buf), fp)) {
    memset(buf, 0, sizeof(buf));
    memset(dns, 0, sizeof(dns));
    if (strstr(buf, "nameserver")) {
      sscanf(buf, "%s%s", name, dns);
    }
    dnslist.push_back(std::string(dns));
  }
  fclose(fp);
  return dnslist;
}

bool setMtu(const std::string &iface, const uint32_t mtu) {
  int fd = -1;
  uint32_t ifru_mtu = 1500;
  struct ifreq ifr;
  struct sockaddr_in broadcastaddr;

  memset(&ifr, 0, sizeof(ifr));
  memset(&broadcastaddr, 0, sizeof(broadcastaddr));

  if (mtu > 1500 || mtu < 100) {
    ifru_mtu = 1500;
  } else {
    ifru_mtu = mtu;
  }

  if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    return false;
  }
  strlcpy(ifr.ifr_name, iface.c_str(), sizeof(ifr.ifr_name));

  bzero(&broadcastaddr, sizeof(struct sockaddr_in));
  broadcastaddr.sin_family = PF_INET;
  ifr.ifr_ifru.ifru_mtu = ifru_mtu;
  if (ioctl(fd, SIOCSIFMTU, &ifr) < 0) {
    close(fd);
    LOG(ERROR) << "set_mtu ioctl error and errno: " << errno;
    return false;
  }
  close(fd);
  return true;
}

bool setSpeed(const std::string &iface, int speed, int duplex, bool autoneg) {
  int fd = -1;
  int err = 0;
  struct ifreq ifr;
  struct ethtool_cmd ecmd;

  memset(&ifr, 0, sizeof(ifr));
  memset(&ecmd, 0, sizeof(ecmd));

  LOG(INFO) << "name = " << iface << " speed = " << speed
            << " duplex = " << duplex << " and autoneg = " << autoneg;

  /* Setup our control structures. */
  memset(&ifr, 0, sizeof(ifr));
  strlcpy(ifr.ifr_name, iface.c_str(), sizeof(ifr.ifr_name));

  /* Open control socket. */
  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    LOG(ERROR) << "Cannot get control socket.";
    return false;
  }

  ecmd.cmd = ETHTOOL_GSET;

  ifr.ifr_data = (caddr_t)&ecmd;
  err = ioctl(fd, SIOCETHTOOL, &ifr);
  if (err < 0) {
    LOG(ERROR) << "Cannot get current device settings.";
  } else {
    if (!autoneg) {
      ecmd.autoneg = AUTONEG_DISABLE;
      /* Change everything the user specified. */
      switch (speed) {
        case 10:
          ecmd.speed = SPEED_10;
          break;
        case 100:
          ecmd.speed = SPEED_100;
          break;
        case 1000:
          ecmd.speed = SPEED_1000;
          break;
        default:
          LOG(ERROR) << "invalid speed mode.";
          close(fd);
          return false;
      }

      if (!duplex) {
        ecmd.duplex = DUPLEX_HALF;
      } else if (1 == duplex) {
        ecmd.duplex = DUPLEX_FULL;
      } else {
        LOG(ERROR) << "invlid duplex mode.";
      }
    } else {
      ecmd.autoneg = AUTONEG_ENABLE;
      ecmd.advertising = ADVERTISED_1000baseT_Half | ADVERTISED_1000baseT_Full |
                         ADVERTISED_100baseT_Full | ADVERTISED_100baseT_Half |
                         ADVERTISED_10baseT_Full | ADVERTISED_10baseT_Half |
                         ADVERTISED_Pause;
    }

    /* Try to perform the update. */
    ecmd.cmd = ETHTOOL_SSET;
    ifr.ifr_data = (caddr_t)&ecmd;
    err = ioctl(fd, SIOCETHTOOL, &ifr);
    if (err < 0) {
      LOG(ERROR) << "Cannot set new settings: " << strerror(errno);
      close(fd);
      return false;
    }
  }
  close(fd);
  return true;
}

bool setActiveRoute(const std::string &iface, const std::string &route,
                    const std::string &mask, bool ishost) {
  int fd = -1;
  struct rtentry rt;
  struct sockaddr_in gateway_addr;

  memset((char *)&rt, 0, sizeof(struct rtentry));
  memset(&gateway_addr, 0, sizeof(gateway_addr));

  if ((fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    return false;
  }

  if (ishost) {
    rt.rt_flags = RTF_UP | RTF_HOST;
  } else {
    rt.rt_flags = RTF_UP;
  }

  bzero(&gateway_addr, sizeof(struct sockaddr_in));
  gateway_addr.sin_family = PF_INET;
  (void)inet_aton(route.c_str(), &gateway_addr.sin_addr);
  memcpy(&rt.rt_dst, &gateway_addr, sizeof(struct sockaddr_in));

  bzero(&gateway_addr, sizeof(struct sockaddr_in));
  (void)inet_aton(mask.c_str(), &gateway_addr.sin_addr);
  gateway_addr.sin_family = PF_INET;
  memcpy(&rt.rt_genmask, &gateway_addr, sizeof(struct sockaddr_in));

  // const char *interface = "eth0";
  rt.rt_dev = (char *)"eth0";

  if (ioctl(fd, SIOCADDRT, &rt) < 0) {
    LOG(ERROR) << "set_route ioctl error and errno: " << errno;
    close(fd);
    return false;
  }
  close(fd);
  return true;
}

bool delActiveRoute(const std::string &iface, const std::string &route,
                    const std::string &mask) {
  int fd = -1;
  struct rtentry rt;
  struct sockaddr_in gateway_addr;

  memset((char *)&rt, 0, sizeof(rt));
  memset((char *)&gateway_addr, 0, sizeof(gateway_addr));

  if ((fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    LOG(ERROR) << "del_route create socket error.";
    return false;
  }

  memset((char *)&rt, 0, sizeof(struct rtentry));
  rt.rt_flags = RTF_UP | RTF_HOST;

  bzero(&gateway_addr, sizeof(struct sockaddr_in));
  gateway_addr.sin_family = PF_INET;
  (void)inet_aton(route.c_str(), &gateway_addr.sin_addr);
  memcpy(&rt.rt_dst, &gateway_addr, sizeof(struct sockaddr_in));

  bzero(&gateway_addr, sizeof(struct sockaddr_in));
  (void)inet_aton(mask.c_str(), &gateway_addr.sin_addr);
  gateway_addr.sin_family = PF_INET;
  memcpy(&rt.rt_genmask, &gateway_addr, sizeof(struct sockaddr_in));

  rt.rt_dev = (char *)iface.c_str();

  if (ioctl(fd, SIOCDELRT, &rt) < 0) {
    LOG(ERROR) << "del_route ioctl error";
    close(fd);
    return false;
  }
  close(fd);
  return true;
}

bool ifaceUp(const std::string &iface) {
  int fd = -1;
  struct ifreq ifr;
  uint16_t flag = 0;

  memset(&ifr, 0, sizeof(ifr));

  if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    return false;
  }

  strlcpy(ifr.ifr_name, iface.c_str(), sizeof(ifr.ifr_name));

  flag = IFF_UP;
  if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
    LOG(ERROR) << "if_up ioctl(SIOCGIFFLAGS) error and errno: " << errno;
    close(fd);
    return false;
  }

  ifr.ifr_ifru.ifru_flags |= flag;
  if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
    LOG(ERROR) << "if_up ioctl(SIOCSIFFLAGS) error and errno: " << errno;
    close(fd);
    return false;
  }

  close(fd);
  return true;
}

bool ifaceDown(const std::string &iface) {
  int fd = -1;
  struct ifreq ifr;
  short flag = 0;

  memset(&ifr, 0, sizeof(ifr));
  if (0 == strcmp(iface.c_str(), "lo")) {
    LOG(ERROR) << "you can't pull down interface lo.";
    return false;
  }

  if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    return false;
  }

  strlcpy(ifr.ifr_name, iface.c_str(), sizeof(ifr.ifr_name));

  flag = ~(IFF_UP);
  if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
    LOG(ERROR) << "if_down ioctl error and errno: " << errno;
    close(fd);
    return false;
  }

  ifr.ifr_ifru.ifru_flags &= flag;
  if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
    LOG(ERROR) << "if_down ioctl error and errno: " << errno;
    close(fd);
    return false;
  }
  close(fd);
  return true;
}

}  // namespace MSF