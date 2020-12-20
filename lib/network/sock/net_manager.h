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
#ifndef SOCK_NET_MANAGER_H_
#define SOCK_NET_MANAGER_H_

#include <unistd.h>

#include <iostream>
#include <vector>

namespace MSF {
/*
 * IPv6 address
 */
typedef struct in6_addr {
  union {
    __uint8_t __u6_addr8[16];
    __uint16_t __u6_addr16[8];
    __uint32_t __u6_addr32[4];
  } __u6_addr; /* 128-bit IP6 address */
} in6_addr_t;

std::string GetGateway(const std::string &iface);

std::string GetNetmask(const std::string &iface);
bool SetNetmask(const std::string &iface, const std::string &netmask);

std::string GetMacAddr(const std::string &iface);
bool SetMacAddr(const std::string &iface, const std::string &macstr);
uint32_t GetMacAddrNum();

std::string GetBroadcastAddr(const std::string &iface);
bool SetBroadcastAddr(const std::string &iface, const std::string &broadcast);

bool addIPv6(const std::string &ipv6, const uint32_t prefix);
bool delIPv6(const std::string &ipv6, const uint32_t prefix);

bool setIPv6Gateway(const std::string &iface, const std::string &gateway);
bool delIPv6Gateway(const struct in6_addr *ipaddr);

bool addIPv4(const std::string &iface, const std::string &ipv4);

bool setIPv4Gateway(const std::string &iface, const std::string &gateway);
bool delIPv4Gateway(void);

std::vector<std::string> getDnsList();

bool setMtu(const std::string &iface, const uint32_t mtu);
bool setSpeed(const std::string &iface, int speed, int duplex, bool autoneg);

bool setActiveRoute(const std::string &iface, const std::string &route,
                    const std::string &mask, bool ishost);
bool delActiveRoute(const std::string &iface, const std::string &route,
                    const std::string &mask);

bool ifaceUp(const std::string &iface);
bool ifaceDown(const std::string &iface);

}  // namespace MSF
#endif