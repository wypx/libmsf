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
#ifndef FRAFT_QUORUM_PEER_H_
#define FRAFT_QUORUM_PEER_H_

#include <string>

namespace MSF {

enum QuorumPeerState {
  LOOKING = 1,
  FOLLOWING = 2,
  LEADING = 3,
  OBSERVING = 4
};

struct QuorumPeer {
 public:
  QuorumPeer(uint64_t server_id, const std::string &server_host,
             uint32_t leader_port, uint32_t election_port)
      : state_(LOOKING),
        server_id_(server_id),
        server_host_(server_host),
        leader_port_(leader_port),
        election_port_(election_port) {}

  ~QuorumPeer() {}

 public:
  QuorumPeerState state_;
  uint64_t server_id_;
  std::string server_host_;
  uint32_t leader_port_;
  uint32_t election_port_;
};
};
#endif  //  FRAFT_QUORUM_PEER_H_
