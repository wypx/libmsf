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
#include <list>
#include <string>
#include <base/logging.h>
#include <base/file.h>

#include <fraft/transaction_log.h>
#include <fraft/transaction_log_iterator.h>
#include <fraft/utility.h>
#include <fraft/string_utility.h>
#include <fraft/proto/record.pb.h>

namespace MSF {

struct TransactionLogIterator::Impl {
 public:
  Impl(const std::string& log_dir, uint64_t gxid);
  ~Impl();
  void Init();
  bool GetNextFileContent();
  bool Next();
  void Close();
  TransactionHeader* header();

  std::list<std::string> sorted_files_;
  std::string log_dir_;
  uint64_t gxid_;
  std::string buffer_;
  TransactionHeader header_;
};

TransactionLogIterator::Impl::Impl(const std::string& log_dir, uint64_t gxid)
    : log_dir_(log_dir), gxid_(gxid) {
  Init();
}

TransactionLogIterator::Impl::~Impl() {}

void TransactionLogIterator::Impl::Init() {
  std::list<std::string> files;
  if (!SortFiles(log_dir_, "log", false, &files)) {
    return;
  }
  std::list<std::string>::iterator iter;
  for (iter = files.begin(); iter != files.end(); ++iter) {
    uint64_t gxid = GetGxidOfFileName(*iter, "log");
    if (gxid >= gxid_) {
      sorted_files_.push_back(*iter);
      continue;
    }
    sorted_files_.push_back(*iter);
    break;
  }
  if (!GetNextFileContent()) {
    return;
  }
  if (!Next()) {
    return;
  }
  while (header_.gxid < gxid_) {
    if (!Next()) {
      return;
    }
  }
}

bool TransactionLogIterator::Impl::GetNextFileContent() {
  if (sorted_files_.empty()) {
    return false;
  }
  std::string file = log_dir_ + sorted_files_.back();
  sorted_files_.pop_back();
  ReadFileContents(file, &buffer_);
  TransactionLogFileHeader file_header;
  file_header.Deserialize(buffer_);
  // skip the file header
  buffer_ = buffer_.substr(FILE_HEADER_SIZE);
  return true;
}

bool TransactionLogIterator::Impl::Next() {
  if (buffer_.length() < TRANSACTION_HEADER_SIZE) {
    if (!GetNextFileContent()) {
      return false;
    }
    return Next();
  }
  if (!header_.Deserialize(buffer_)) {
    return false;
  }
  buffer_ = buffer_.substr(TRANSACTION_HEADER_SIZE + header_.record_length);
  return true;
}

void TransactionLogIterator::Impl::Close() { sorted_files_.clear(); }

TransactionHeader* TransactionLogIterator::Impl::header() { return &header_; }

TransactionLogIterator::TransactionLogIterator(const std::string& log_dir,
                                               uint64_t gxid) {
  impl_ = new Impl(log_dir, gxid);
}

TransactionLogIterator::~TransactionLogIterator() { delete impl_; }

bool TransactionLogIterator::Next() { return impl_->Next(); }

void TransactionLogIterator::Close() { impl_->Close(); }

TransactionHeader* TransactionLogIterator::header() { return impl_->header(); }
};
