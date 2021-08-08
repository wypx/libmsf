/*
 * Copyright (C) Lichuang
 */
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <gtest/gtest.h>
#include <list>
#include <string>
#include "global/utility.h"

using namespace std;
namespace global {
class UtilityTest : public testing::Test {
 public:
  void SetUp() {
    tmp_dir_ = "/tmp/test_dir/";
    ASSERT_EQ(0, mkdir(tmp_dir_.c_str(), S_IRWXU));
  }

  void TearDown() {
    rmdir(tmp_dir_.c_str());
  }
  string tmp_dir_;
};

TEST_F(UtilityTest, SortFilesTest) {
  list<string> files;
  ASSERT_FALSE(SortFiles(tmp_dir_, "log", true, &files));
  string file1 = "log.1";
  string file1_full_name = tmp_dir_ + "log.1";
  ASSERT_EQ(1u, GetGxidOfFileName(file1, "log"));
  FILE *file = fopen(file1_full_name.c_str(), "w");
  ASSERT_TRUE(file != NULL);
  fclose(file);
  string file2 = "log.2";
  string file2_full_name = tmp_dir_ + "log.2";
  file = fopen(file2_full_name.c_str(), "w");
  ASSERT_TRUE(file != NULL);
  fclose(file);
  ASSERT_TRUE(SortFiles(tmp_dir_, "log", true, &files));
  ASSERT_EQ(2u, files.size());
  ASSERT_EQ(file1, *(files.begin()));
  remove(file1_full_name.c_str());
  remove(file2_full_name.c_str());
}
};

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
