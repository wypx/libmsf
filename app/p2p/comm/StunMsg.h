

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
#ifndef P2P_COMM_STUN_H
#define P2P_COMM_STUN_H

#include <string>

namespace MSF {
namespace P2P {

/* STUN magic cookie. */
#define STUN_MAGIC  0x2112A442

/**
   Defines a STUN Message object
   <pre>
   .---------------------.      /|\           /|\
   | STUN Header         |       |             |
   |---------------------|       |             |
   |         ....        |       |--------.    |
   |      N Attributes   |       |        |    |----.
   |         ....        |      \|/       |    |    |
   |---------------------|                |    |    |
   |  MESSAGE-INTEGRITY  | <-(HMAC-SHA1)--'   \|/   |
   |---------------------|                          |
   |     FINGERPRINT     | <-(CRC-32)---------------'
   '---------------------'
   </pre>
*/

struct StunMsg {

};

}
}
#endif