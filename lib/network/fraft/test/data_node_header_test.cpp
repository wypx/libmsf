/*
 * Copyright (C) Lichuang
 */
#include <string>
#include <gtest/gtest.h>
#include <eventrpc/file_utility.h>
#include "global/data_node_header.h"
using namespace std;
namespace global {
class DataNodeHeaderTest : public testing::Test {
 public:
  void SetUp() {
  }

  void TearDown() {
  }
};

TEST_F(DataNodeHeaderTest, SerializeTest) {
  DataNodeHeader header, result;
  string content;
  header.node_size = 100;
  ASSERT_TRUE(header.Serialize(&content));
  ASSERT_TRUE(result.Deserialize(content));
  ASSERT_EQ(header.node_size, result.node_size);
}
};

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
