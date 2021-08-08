/*
 * Copyright (C) Lichuang
 */
#include <iostream>
#include <gtest/gtest.h>
#include <eventrpc/file_utility.h>
#include "global/data_tree.h"

using namespace eventrpc;
namespace global {
class DataTreeTest : public testing::Test {
 public:
  void SetUp() {
  }

  void TearDown() {
  }
  void InitDataNode(global::NodeData *node);
};

void DataTreeTest::InitDataNode(global::NodeData *node) {
  node->set_permission(1);
  node->set_owner(2);
  global::NodeStat *stat = node->mutable_stat();
  stat->set_cgxid(1);
  stat->set_mgxid(1);
  stat->set_ctime(1);
  stat->set_mtime(1);
  stat->set_version(1);
  stat->set_cversion(1);
  stat->set_aversion(1);
  stat->set_ephemeral_owner(1);
  stat->set_pgxid(1);
}

TEST_F(DataTreeTest, AddTest) {
  DataTree data_tree, result;
  list<string> children;
  ASSERT_TRUE(data_tree.GetChildren("/", &children));
  ASSERT_TRUE(children.empty());

  global::NodeData data;
  InitDataNode(&data);
  data.set_path("/test");
  ASSERT_TRUE(data_tree.AddNode(data));
  ASSERT_TRUE(data_tree.GetChildren("/", &children));
  ASSERT_FALSE(children.empty());
  ASSERT_EQ(1u, children.size());
  ASSERT_EQ("test", *(children.begin()));

  TreeNode *node = NULL;
  node = data_tree.GetNode("/test");
  ASSERT_EQ("/", node->parent->node_data.path());

  data.set_path("/test/1");
  ASSERT_TRUE(data_tree.AddNode(data));
  ASSERT_TRUE(data_tree.GetChildren("/", &children));
  ASSERT_FALSE(children.empty());
  ASSERT_EQ(1u, children.size());
  ASSERT_TRUE(data_tree.GetChildren("/test", &children));
  ASSERT_EQ(1u, children.size());
  ASSERT_EQ("1", *(children.begin()));
  node = data_tree.GetNode("/test/1");
  ASSERT_EQ("/test", node->parent->node_data.path());

  node = data_tree.SetData("/test/1", "data", 1, 2, 3);
  ASSERT_EQ("data", node->node_data.data());

  string content;
  data_tree.Serialize(&content);
  result.Deserialize(content);
  node = result.GetNode("/test/1");
  ASSERT_EQ("/test", node->parent->node_data.path());

  ASSERT_TRUE(data_tree.DeleteNode("/test/1", 0));
  ASSERT_TRUE(data_tree.GetChildren("/test", &children));
  ASSERT_EQ(0u, children.size());

  data.set_path("test/1");
  ASSERT_FALSE(data_tree.AddNode(data));
}
};

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
