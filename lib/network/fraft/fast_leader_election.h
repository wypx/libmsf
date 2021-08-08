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
#ifndef FRAFT_FAST_ELECTION_H_
#define FRAFT_FAST_ELECTION_H_

#include <string>

namespace MSF {

class QuorumPeerServer;

class FastLeaderElection {
 public:
  explicit FastLeaderElection(QuorumPeerServer *server);
  ~FastLeaderElection();

  void LookForLeader();

 private:
  uint64_t logical_lock_;
  QuorumPeerServer *quorum_peer_server_;
};
};
#endif  // FRAFT_FAST_ELECTION_H_
