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
#ifndef FRAFT_TRANSACTION_LOG_H_
#define FRAFT_TRANSACTION_LOG_H_

#include <list>
#include <string>
#include <fraft/transaction_log_header.h>
#include <fraft/proto/record.pb.h>

namespace MSF {

class TransactionLogIterator;
class TransactionLog {
 public:
  explicit TransactionLog(const std::string &log_dir);
  ~TransactionLog();

  void Roll();

  bool Append(const TransactionHeader &header,
              const ::google::protobuf::Message *message);

  TransactionLogIterator *Read(uint64_t gxid);

  uint64_t GetLastLoggedGxid() const;

  bool GetLogFiles(std::list<std::string> *files, uint64_t gxid) const;

  bool Truncate(uint64_t gxid);

  uint64_t dbid() const;

  void Commit();

  void Close();

 private:
  struct Impl;
  Impl *impl_;
};
};
#endif  // FRAFT_TRANSACTION_LOG_H_
