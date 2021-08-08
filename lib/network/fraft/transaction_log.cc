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
#include <stdio.h>
#include <fraft/transaction_log.h>
#include <fraft/transaction_log_iterator.h>
#include <fraft/utility.h>
#include <fraft/string_utility.h>

namespace {
static const char kLogFileHeaderMagic[] = "GTLOG";
static const uint32_t kLogVersion = 1;
static const uint64_t kDbId = 1;
};

namespace MSF {

struct TransactionLog::Impl {
 public:
  explicit Impl(const std::string &log_dir);
  ~Impl();

  void Roll();

  bool Append(const TransactionHeader &header,
              const ::google::protobuf::Message *message);

  TransactionLogIterator *Read(uint64_t gxid);

  uint64_t GetLastLoggedGxid() const;

  uint64_t dbid() const;

  void Commit();

  void Close();

 private:
  std::string log_dir_;
  FILE *file_;
  TransactionLogFileHeader file_header_;
  std::string file_header_string_;
};

TransactionLog::Impl::Impl(const std::string &log_dir)
    : log_dir_(log_dir), file_(NULL) {
  file_header_.magic = atol(kLogFileHeaderMagic);
  file_header_.version = kLogVersion;
  file_header_.dbid = kDbId;
  file_header_.Serialize(&file_header_string_);
}

TransactionLog::Impl::~Impl() { Close(); }

void TransactionLog::Impl::Roll() {
  if (!file_) {
    return;
  }
  fflush(file_);
  fclose(file_);
  file_ = NULL;
}

bool TransactionLog::Impl::Append(const TransactionHeader &header,
                                  const ::google::protobuf::Message *message) {
  if (file_ == NULL) {
    std::string filename = log_dir_ + "/log.";
    filename += StringUtility::ConvertUint64ToString(header.gxid);
    file_ = fopen(filename.c_str(), "a");
    fwrite(file_header_string_.c_str(), sizeof(char),
           file_header_string_.length(), file_);
  }
  std::string output;
  header.Serialize(&output);
  fwrite(output.c_str(), sizeof(char), output.length(), file_);
  output = "";
  message->SerializeToString(&output);
  fwrite(output.c_str(), sizeof(char), output.length(), file_);
  return true;
}

TransactionLogIterator *TransactionLog::Impl::Read(uint64_t gxid) {
  return new TransactionLogIterator(log_dir_, gxid);
}

uint64_t TransactionLog::Impl::GetLastLoggedGxid() const {
  std::list<std::string> files;
  if (!SortFiles(log_dir_, "log", false, &files)) {
    return 0;
  }
  std::string last_file = *(files.begin());
  uint64_t gxid = GetGxidOfFileName(last_file, "log");
  TransactionLogIterator iter(log_dir_, gxid);
  while (true) {
    if (!iter.Next()) {
      break;
    }
    TransactionHeader *header = iter.header();
    gxid = header->gxid;
  }
  return gxid;
}

uint64_t TransactionLog::Impl::dbid() const { return kDbId; }

void TransactionLog::Impl::Commit() {
  if (file_) {
    fflush(file_);
  }
}

void TransactionLog::Impl::Close() { Roll(); }

TransactionLog::TransactionLog(const std::string &log_dir)
    : impl_(new Impl(log_dir)) {}

TransactionLog::~TransactionLog() { delete impl_; }

void TransactionLog::Roll() { impl_->Roll(); }

bool TransactionLog::Append(const TransactionHeader &header,
                            const ::google::protobuf::Message *message) {
  return impl_->Append(header, message);
}

TransactionLogIterator *TransactionLog::Read(uint64_t gxid) {
  return impl_->Read(gxid);
}

uint64_t TransactionLog::GetLastLoggedGxid() const {
  return impl_->GetLastLoggedGxid();
}

uint64_t TransactionLog::dbid() const { return impl_->dbid(); }

void TransactionLog::Commit() { impl_->Commit(); }

void TransactionLog::Close() { impl_->Close(); }
};
