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
#ifndef FRAFT_DATA_TREE_H_
#define FRAFT_DATA_TREE_H_

#include <stdint.h>
#include <string>
#include <list>
#include <set>
#include <fraft/proto/transaction.pb.h>

namespace MSF {

struct TreeNode {
  struct TreeNode *parent;
  fraft::NodeData node_data;
  std::set<std::string> children;
  TreeNode(fraft::NodeData data) : parent(NULL), node_data(data) {}
};
class DataTree {
 public:
  DataTree();
  ~DataTree();

  bool Deserialize(const std::string &input);
  bool Serialize(std::string *output) const;

  bool AddNode(const fraft::NodeData &node);
  TreeNode *GetNode(const std::string &path);
  bool DeleteNode(const std::string &path, uint64_t gxid);
  TreeNode *SetData(const std::string &path, const std::string &data,
                    uint32_t version, uint32_t gxid, uint64_t time);
  bool GetChildren(const std::string &path,
                   std::list<std::string> *children) const;

 private:
  struct Impl;
  Impl *impl_;
};
};
#endif
