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
#include <base/logging.h>
#include "quorum_peer_server.h"
#include "fast_leader_election.h"

namespace MSF {

FastLeaderElection::FastLeaderElection(QuorumPeerServer *server)
    : logical_lock_(0), quorum_peer_server_(server) {}

FastLeaderElection::~FastLeaderElection() {}

void FastLeaderElection::LookForLeader() {}
};
