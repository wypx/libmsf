/*
 * Copyright (C) Lichuang
 */
#include <gtest/gtest.h>
#include <eventrpc/file_utility.h>
#include "global/transaction_log_iterator.h"
#include "global/transaction_log.h"

namespace global {
class TransactionLogIteratorTest : public testing::Test {
 public:
  void SetUp() {
    tmp_dir_ = "/tmp/test_dir/";
    ASSERT_EQ(0, mkdir(tmp_dir_.c_str(), S_IRWXU));
    CreateTestLogFile();
  }

  void TearDown() {
    DeleteTestLogFile();
    rmdir(tmp_dir_.c_str());
  }

  void CreateTestLogFile();
  void DeleteTestLogFile();

  string tmp_dir_;
};

void TransactionLogIteratorTest::CreateTestLogFile() {
  {
    TransactionLog log(tmp_dir_);
    {
      global::TransactionHeader header;
      header.client_id = 1;
      header.cxid = 1;
      header.gxid = 1;
      header.time = 1;
      header.type = 1;
      header.checksum = 1;
      global::Delete record;
      record.set_path("test");
      header.record_length = record.ByteSize();
      log.Append(header, &record);
      log.Commit();
    }
    {
      global::TransactionHeader header;
      header.client_id = 1;
      header.cxid = 1;
      header.gxid = 2;
      header.time = 1;
      header.type = 1;
      header.checksum = 1;
      global::Delete record;
      record.set_path("test2");
      header.record_length = record.ByteSize();
      log.Append(header, &record);
      log.Commit();
    }
  }
  {
    TransactionLog log(tmp_dir_);
    {
      global::TransactionHeader header;
      header.client_id = 1;
      header.cxid = 1;
      header.gxid = 3;
      header.time = 1;
      header.type = 1;
      header.checksum = 1;
      global::Delete record;
      record.set_path("test");
      header.record_length = record.ByteSize();
      log.Append(header, &record);
      log.Commit();
    }
    {
      global::TransactionHeader header;
      header.client_id = 1;
      header.cxid = 1;
      header.gxid = 4;
      header.time = 1;
      header.type = 1;
      header.checksum = 1;
      global::Delete record;
      record.set_path("test2");
      header.record_length = record.ByteSize();
      log.Append(header, &record);
      log.Commit();
    }
  }
}

void TransactionLogIteratorTest::DeleteTestLogFile() {
  string file = tmp_dir_ + "/log.1";
  remove(file.c_str());
  file = tmp_dir_ + "/log.3";
  remove(file.c_str());
}

TEST_F(TransactionLogIteratorTest, AppendTest) {
  global::TransactionHeader *header;
  TransactionLogIterator iter(tmp_dir_, 1);
  header = iter.header();
  ASSERT_EQ(1u, header->gxid);
  iter.Next();
  header = iter.header();
  ASSERT_EQ(2u, header->gxid);
  iter.Next();
  header = iter.header();
  ASSERT_EQ(3u, header->gxid);
}
};

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
