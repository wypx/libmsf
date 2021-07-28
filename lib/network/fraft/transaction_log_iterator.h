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
#ifndef FRAFT_TRANSACTION_LOG_ITERATOR_H_
#define FRAFT_TRANSACTION_LOG_ITERATOR_H_

#include <fraft/transaction_log_header.h>

namespace MSF {
class TransactionLogIterator {
 public:
  TransactionLogIterator(const std::string &log_dir, uint64_t gxid);
  ~TransactionLogIterator();
  bool Next();
  void Close();
  TransactionHeader *header();

 private:
  struct Impl;
  Impl *impl_;
};
};
#endif  // FRAFT_TRANSACTION_LOG_ITERATOR_H_
