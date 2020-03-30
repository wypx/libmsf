
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
#ifndef P2P_COMM_NATDETECT_H
#define P2P_COMM_NATDETECT_H

#include <string>

namespace MSF {
namespace P2P {


/**
 * This enumeration describes the NAT types, as specified by RFC 3489
 * Section 5, NAT Variations.
 */
enum StunNatType {
  StunTypeUnknown = 0, /* NAT type is unknown because the detection has not been performed */
  StunTypeFailure, /* NAT type is unknown because there is failure in the detection
                    * process, possibly because server does not support RFC 3489 */
  StunTypeOpen,    /* This specifies that the client has open access to Internet (or
                    * at least, its behind a firewall that behaves like a full-cone NAT,
                    * but without the translation) */
  StunTypeBlocked, /* This specifies that communication with server has failed, probably
                    * because UDP packets are blocked. */
  StunTypeFullConeNat, /* A full cone NAT is one where all requests from the same internal 
                        * IP address and port are mapped to the same external IP address and
                        * port.  Furthermore, any external host can send a packet to the 
                        * internal host, by sending a packet to the mapped external address.*/
  StunTypeRestrictedNat, /* A restricted cone NAT is one where all requests from the same 
                          * internal IP address and port are mapped to the same external IP 
                          * address and port.  Unlike a full cone NAT, an external host (with 
                          * IP address X) can send a packet to the internal host only if the 
                          * internal host had previously sent a packet to IP address X.*/
  StunTypePortRestrictedNat, /* A port restricted cone NAT is like a restricted cone NAT, but the 
                              * restriction includes port numbers. Specifically, an external host 
                              * can send a packet, with source IP address X and source port P, 
                              * to the internal host only if the internal host had previously sent
                              * a packet to IP address X and port P.*/
  StunTypeSymmetricNat,  /* A symmetric NAT is one where all requests from the same internal 
                          * IP address and port, to a specific destination IP address and port,
                          * are mapped to the same external IP address and port.  If the same 
                          * host sends a packet with the same source address and port, but to 
                          * a different destination, a different mapping is used.  Furthermore,
                          * only the external host that receives a packet can send a UDP packet
                          * back to the internal host.*/
  StunTypeSymmetricUdp, /* Firewall that allows UDP out, and responses have to come back to
                         * the source of the request (like a symmetric NAT, but no translation.*/
  StunTypeSymFirewall,
};


struct StunNatDectectResp {
  int status_;
  std::string statusText_;
  enum StunNatType natType_;
  std::string natTypeName_;
};

class StunNatDetect {
  private:

  private:
    static std::string GetNatTypeName(enum StunNatType natType);

  public:
};

}
}
#endif