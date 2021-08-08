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
#include <map>
#include <google/protobuf/text_format.h>
#include <base/logging.h>
#include <base/file.h>
#include <fraft/quorum_peer_manager.h>

namespace MSF {

struct QuorumPeerManager::Impl {
 public:
  Impl();
  ~Impl();

  uint64_t my_server_id() const;

  bool ParseConfigFile(const std::string &config_file);

  QuorumPeer *FindQuorumPeerById(uint64_t server_id);

  void GetOtherQuorumPeers(std::list<QuorumPeer *> *quorum_peers);

  // void set_dispatcher(eventrpc::Dispatcher *dispatcher) {
  //   dispatcher_ = dispatcher;
  // }

 private:
  std::map<uint64_t, QuorumPeer *> quorum_peer_map_;
  fraft::ServerConfig server_config_;
  // eventrpc::Dispatcher *dispatcher_;
};

QuorumPeerManager::Impl::Impl() {}

QuorumPeerManager::Impl::~Impl() {
  std::map<uint64_t, QuorumPeer *>::iterator iter;
  for (iter = quorum_peer_map_.begin(); iter != quorum_peer_map_.end();) {
    QuorumPeer *peer = iter->second;
    ++iter;
    delete peer;
  }
  quorum_peer_map_.clear();
}

QuorumPeer *QuorumPeerManager::Impl::FindQuorumPeerById(uint64_t server_id) {
  std::map<uint64_t, QuorumPeer *>::iterator iter;
  iter = quorum_peer_map_.find(server_id);
  if (iter == quorum_peer_map_.end()) {
    return NULL;
  }
  return iter->second;
}

void QuorumPeerManager::Impl::GetOtherQuorumPeers(
    std::list<QuorumPeer *> *quorum_peers) {
  std::map<uint64_t, QuorumPeer *>::iterator iter;
  for (iter = quorum_peer_map_.begin(); iter != quorum_peer_map_.end();
       ++iter) {
    if (iter->first == server_config_.my_server_id()) {
      continue;
    }
    quorum_peers->push_back(iter->second);
  }
}

uint64_t QuorumPeerManager::Impl::my_server_id() const {
  return server_config_.my_server_id();
}

bool QuorumPeerManager::Impl::ParseConfigFile(const std::string &config_file) {
  std::string content;
  if (!ReadFileContents(config_file, &content)) {
    LOG(ERROR) << "cannot read config file " << config_file;
    return false;
  }
  if (!google::protobuf::TextFormat::ParseFromString(content,
                                                     &server_config_)) {
    LOG(ERROR) << "parse file " << config_file << " error: \n" << content;
    return false;
  }
  uint64_t server_id = 0;
  for (int i = 0; i < server_config_.server_info_size(); ++i) {
    const fraft::ServerInfo &server_info = server_config_.server_info(i);
    server_id = server_info.server_id();
    QuorumPeer *peer =
        new QuorumPeer(server_id, server_info.server_host(),
                       server_info.leader_port(), server_info.election_port());
    quorum_peer_map_[server_id] = peer;
  }
  return true;
}

QuorumPeerManager::QuorumPeerManager() : impl_(new Impl) {}

QuorumPeerManager::~QuorumPeerManager() { delete impl_; }

QuorumPeer *QuorumPeerManager::FindQuorumPeerById(uint64_t server_id) {
  return impl_->FindQuorumPeerById(server_id);
}

void QuorumPeerManager::GetOtherQuorumPeers(
    std::list<QuorumPeer *> *quorum_peers) {
  impl_->GetOtherQuorumPeers(quorum_peers);
}

// void QuorumPeerManager::set_dispatcher(eventrpc::Dispatcher *dispatcher) {
//   impl_->set_dispatcher(dispatcher);
// }

uint64_t QuorumPeerManager::my_server_id() const {
  return impl_->my_server_id();
}

bool QuorumPeerManager::ParseConfigFile(const std::string &config_file) {
  return impl_->ParseConfigFile(config_file);
}
};
