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
#include <fraft/data_tree.h>
#include <fraft/data_node_header.h>

namespace MSF {

struct DataTree::Impl {
 public:
  Impl();
  ~Impl();

  bool Deserialize(const std::string &input);
  bool Serialize(std::string *output) const;

  bool AddNode(const fraft::NodeData &node);
  TreeNode *GetParentNode(const std::string &path);
  std::string GetChildPath(TreeNode *parent_node, const std::string &path);
  TreeNode *GetNode(const std::string &path);
  bool DeleteNode(const std::string &path, uint64_t gxid);
  TreeNode *SetData(const std::string &path, const std::string &data,
                    uint32_t version, uint32_t gxid, uint64_t time);
  bool GetData(const std::string &path, const std::string *data) const;
  bool GetChildren(const std::string &path,
                   std::list<std::string> *children) const;

 private:
  typedef std::map<std::string, TreeNode *> NodeMap;
  NodeMap nodes_;
  TreeNode *root_node_;
};

DataTree::Impl::Impl() {
  fraft::NodeData node;
  node.set_path("/");
  node.set_permission(1);
  node.set_owner(2);
  fraft::NodeStat *stat = node.mutable_stat();
  stat->set_cgxid(1);
  stat->set_mgxid(1);
  stat->set_ctime(1);
  stat->set_mtime(1);
  stat->set_version(1);
  stat->set_cversion(1);
  stat->set_aversion(1);
  stat->set_ephemeral_owner(1);
  stat->set_pgxid(1);
  root_node_ = new TreeNode(node);
  nodes_["/"] = root_node_;
}

DataTree::Impl::~Impl() {
  NodeMap::iterator iter;
  for (iter = nodes_.begin(); iter != nodes_.end();) {
    TreeNode *node = iter->second;
    ++iter;
    delete node;
  }
}

bool DataTree::Impl::Deserialize(const std::string &input) {
  DataNodeHeader header;
  uint32_t pos = 0, size = 0;
  while (true) {
    if (!header.Deserialize(input.substr(pos))) {
      break;
    }
    pos += DATA_NODE_HEADER_SIZE;
    size = header.node_size;
    fraft::NodeData node_data;
    if (!node_data.ParseFromString(input.substr(pos, size))) {
      return false;
    }
    AddNode(node_data);
    pos += size;
  }
  return true;
}

bool DataTree::Impl::Serialize(std::string *output) const {
  NodeMap::const_iterator iter;
  for (iter = nodes_.begin(); iter != nodes_.end(); ++iter) {
    TreeNode *node = iter->second;
    DataNodeHeader header;
    header.node_size = node->node_data.ByteSizeLong();
    std::string result;
    header.Serialize(&result);
    output->append(result);
    result = "";
    node->node_data.SerializeToString(&result);
    output->append(result);
  }
  return true;
}

TreeNode *DataTree::Impl::GetParentNode(const std::string &path) {
  std::string::size_type pos = path.find_last_of("/");
  if (pos == std::string::npos) {
    LOG(ERROR) << "path " << path << " has no parent";
    return NULL;
  }
  TreeNode *parent_node;
  if (pos == 0) {
    if (path == "/") {
      LOG(ERROR) << "root path not allow modify";
      return NULL;
    }
    parent_node = root_node_;
  } else {
    std::string parent_path = path.substr(0, pos);
    NodeMap::iterator iter = nodes_.find(parent_path);
    if (iter == nodes_.end()) {
      LOG(ERROR) << "path " << path << " has no parent"
                 << ", parent path: " << parent_path;
      return NULL;
    }
    parent_node = iter->second;
  }
  return parent_node;
}

std::string DataTree::Impl::GetChildPath(TreeNode *parent_node,
                                         const std::string &path) {
  uint32_t parent_path_length = parent_node->node_data.path().length();
  if (parent_path_length != 1) {
    ++parent_path_length;
  }
  return path.substr(parent_path_length);
}

bool DataTree::Impl::AddNode(const fraft::NodeData &node) {
  std::string path = node.path();
  NodeMap::iterator iter = nodes_.find(path);
  if (iter != nodes_.end()) {
    LOG(ERROR) << "path " << path << " node existed";
    return false;
  }
  TreeNode *parent_node = GetParentNode(path);
  if (parent_node == NULL) {
    return false;
  }
  TreeNode *tree_node = new TreeNode(node);
  nodes_[node.path()] = tree_node;
  std::string child_path = GetChildPath(parent_node, path);
  tree_node->parent = parent_node;
  parent_node->children.insert(child_path);
  return true;
}

TreeNode *DataTree::Impl::GetNode(const std::string &path) {
  NodeMap::iterator iter = nodes_.find(path);
  if (iter == nodes_.end()) {
    LOG(ERROR) << "path " << path << " not exist";
    return NULL;
  }
  return iter->second;
}

bool DataTree::Impl::DeleteNode(const std::string &path, uint64_t gxid) {
  TreeNode *parent_node = GetParentNode(path);
  if (NULL == parent_node) {
    return false;
  }
  NodeMap::iterator iter = nodes_.find(path);
  if (iter == nodes_.end()) {
    LOG(ERROR) << "path " << path << " not exist";
    return false;
  }
  std::string child_path = GetChildPath(parent_node, path);
  parent_node->children.erase(child_path);
  parent_node->node_data.mutable_stat()->set_pgxid(gxid);
  parent_node->node_data.mutable_stat()->set_cversion(
      parent_node->node_data.stat().cversion() + 1);
  nodes_.erase(iter);
  return true;
}

TreeNode *DataTree::Impl::SetData(const std::string &path,
                                  const std::string &data, uint32_t version,
                                  uint32_t gxid, uint64_t time) {
  TreeNode *node = GetNode(path);
  if (node == NULL) {
    return NULL;
  }
  node->node_data.set_data(data);
  node->node_data.mutable_stat()->set_mtime(time);
  node->node_data.mutable_stat()->set_version(version);
  node->node_data.mutable_stat()->set_mgxid(gxid);
  return node;
}

bool DataTree::Impl::GetChildren(const std::string &path,
                                 std::list<std::string> *children) const {
  children->clear();
  NodeMap::const_iterator iter = nodes_.find(path);
  if (iter == nodes_.end()) {
    LOG(ERROR) << "path " << path << " not exist";
    return false;
  }
  const TreeNode *tree_node = iter->second;
  std::set<std::string>::iterator child_iter = tree_node->children.begin();
  for (; child_iter != tree_node->children.end(); ++child_iter) {
    children->push_back(*child_iter);
  }
  return true;
}

DataTree::DataTree() : impl_(new Impl) {}

DataTree::~DataTree() { delete impl_; }

bool DataTree::Deserialize(const std::string &input) {
  return impl_->Deserialize(input);
}

bool DataTree::Serialize(std::string *output) const {
  return impl_->Serialize(output);
}

bool DataTree::AddNode(const fraft::NodeData &node) {
  return impl_->AddNode(node);
}

TreeNode *DataTree::GetNode(const std::string &path) {
  return impl_->GetNode(path);
}

bool DataTree::DeleteNode(const std::string &path, uint64_t gxid) {
  return impl_->DeleteNode(path, gxid);
}

TreeNode *DataTree::SetData(const std::string &path, const std::string &data,
                            uint32_t version, uint32_t gxid, uint64_t time) {
  return impl_->SetData(path, data, version, gxid, time);
}

bool DataTree::GetChildren(const std::string &path,
                           std::list<std::string> *children) const {
  return impl_->GetChildren(path, children);
}
};
