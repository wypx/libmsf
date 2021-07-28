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
#ifndef FRAFTL_QORUM_PEER_SERVER_H_
#define FRAFTL_QORUM_PEER_SERVER_H_

namespace MSF {

class QuorumPeerManager;
class DataTree;
class QuorumPeerServer {
 public:
  QuorumPeerServer();
  ~QuorumPeerServer();

  void set_quorumpeer_manager(QuorumPeerManager *manager);
  void set_data_tree(DataTree *data_tree);
  // void set_dispatcher(eventrpc::Dispatcher *dispatcher);

  void Start();

  struct Impl;

 private:
  Impl *impl_;
};
};
#endif  // FRAFT_QUORUM_PEER_SERVER_H_
