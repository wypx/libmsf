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
#ifndef FRAFT_TRANSACTION_HEADER_H_
#define FRAFT_TRANSACTION_HEADER_H_

#include <string>

namespace MSF {

enum RecordType {
  CREATE = 1,
};
struct TransactionLogFileHeader {
  uint32_t magic;
  uint32_t version;
  uint32_t dbid;

  TransactionLogFileHeader() : magic(0), version(0), dbid(0) {}
  bool Serialize(std::string *output) const;
  bool Deserialize(const std::string &input);
};

struct SnapLogFileHeader {
  uint32_t magic;
  uint32_t version;
  uint32_t dbid;
  uint32_t session_size;

  SnapLogFileHeader() : magic(0), version(0), dbid(0), session_size(0) {}
  bool Serialize(std::string *output) const;
  bool Deserialize(const std::string &input);
};

struct TransactionHeader {
  uint64_t client_id;
  uint32_t cxid;
  uint64_t gxid;
  uint64_t time;
  uint32_t type;
  uint32_t checksum;
  uint32_t record_length;
  TransactionHeader()
      : client_id(0),
        cxid(0),
        gxid(0),
        time(0),
        type(0),
        checksum(0),
        record_length(0) {}
  bool Serialize(std::string *output) const;
  bool Deserialize(const std::string &input);
};

#define FILE_HEADER_SIZE sizeof(TransactionLogFileHeader)
#define TRANSACTION_HEADER_SIZE sizeof(TransactionHeader)
#define SNAP_LOG_HEADER_SIZE sizeof(SnapLogFileHeader)
};
#endif  // FRAFT_TRANSACTION_HEADER_H_
