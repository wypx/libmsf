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
#ifndef FRAFT_QUORUM_PEER_MANAGER_H_
#define FRAFT_QUORUM_PEER_MANAGER_H_

#include <list>
#include <fraft/proto/server_config.pb.h>
#include <fraft/quorum_peer.h>

namespace MSF {

class QuorumPeer;
class QuorumPeerManager {
 public:
  explicit QuorumPeerManager();
  ~QuorumPeerManager();

  uint64_t my_server_id() const;

  bool ParseConfigFile(const std::string &config_file);

  QuorumPeer *FindQuorumPeerById(uint64_t server_id);

  void GetOtherQuorumPeers(std::list<QuorumPeer *> *quorum_peers);

  // void set_dispatcher(eventrpc::Dispatcher *dispatcher);

  bool SendMessage(uint32_t server_id, ::google::protobuf::Message *message);

  struct Impl;

 private:
  Impl *impl_;
};
};
#endif  // FRAFT_QUORUM_PEER_MANAGER_H_
