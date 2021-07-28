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
#include <base/logging.h>
#include <fraft/string_utility.h>
#include <fraft/transaction_log_header.h>

namespace MSF {

bool TransactionLogFileHeader::Serialize(std::string *output) const {
  // uint32_t magic
  output->append(StringUtility::SerializeUint32ToString(magic));
  // uint32_t version
  output->append(StringUtility::SerializeUint32ToString(version));
  // uint32_t dbid
  output->append(StringUtility::SerializeUint32ToString(dbid));
  return true;
}

bool TransactionLogFileHeader::Deserialize(const std::string &input) {
  uint32_t pos = 0;
  uint32_t size = sizeof(uint32_t);
  magic = StringUtility::DeserializeStringToUint32(input.substr(pos, size));
  pos += size;
  version = StringUtility::DeserializeStringToUint32(input.substr(pos, size));
  pos += size;
  dbid = StringUtility::DeserializeStringToUint32(input.substr(pos));
  return true;
}

bool SnapLogFileHeader::Serialize(std::string *output) const {
  // uint32_t magic
  output->append(StringUtility::SerializeUint32ToString(magic));
  // uint32_t version
  output->append(StringUtility::SerializeUint32ToString(version));
  // uint32_t dbid
  output->append(StringUtility::SerializeUint32ToString(dbid));
  // uint32_t session_size
  output->append(StringUtility::SerializeUint32ToString(session_size));
  return true;
}

bool SnapLogFileHeader::Deserialize(const std::string &input) {
  uint32_t pos = 0;
  uint32_t size = sizeof(uint32_t);
  magic = StringUtility::DeserializeStringToUint32(input.substr(pos, size));
  pos += size;
  version = StringUtility::DeserializeStringToUint32(input.substr(pos, size));
  pos += size;
  dbid = StringUtility::DeserializeStringToUint32(input.substr(pos));
  pos += size;
  session_size = StringUtility::DeserializeStringToUint32(input.substr(pos));
  return true;
}
bool TransactionHeader::Serialize(std::string *output) const {
  // uint64_t client id
  output->append(StringUtility::SerializeUint64ToString(client_id));
  // uint32_t cxid
  output->append(StringUtility::SerializeUint32ToString(cxid));
  // uint64_t gxid
  output->append(StringUtility::SerializeUint64ToString(gxid));
  // uint64_t time
  output->append(StringUtility::SerializeUint64ToString(time));
  // uint32_t type
  output->append(StringUtility::SerializeUint32ToString(type));
  // uint32_t checksum
  output->append(StringUtility::SerializeUint32ToString(checksum));
  // uint32_t record_length
  output->append(StringUtility::SerializeUint32ToString(record_length));
  return true;
}

bool TransactionHeader::Deserialize(const std::string &input) {
  uint32_t pos = 0;
  uint32_t size = sizeof(uint64_t);
  client_id = StringUtility::DeserializeStringToUint64(input.substr(pos, size));

  pos += size;
  size = sizeof(uint32_t);
  cxid = StringUtility::DeserializeStringToUint32(input.substr(pos, size));

  pos += size;
  size = sizeof(uint64_t);
  gxid = StringUtility::DeserializeStringToUint64(input.substr(pos, size));

  pos += size;
  size = sizeof(uint64_t);
  time = StringUtility::DeserializeStringToUint64(input.substr(pos, size));

  pos += size;
  size = sizeof(uint32_t);
  type = StringUtility::DeserializeStringToUint32(input.substr(pos, size));

  pos += size;
  size = sizeof(uint32_t);
  checksum = StringUtility::DeserializeStringToUint32(input.substr(pos, size));

  pos += size;
  size = sizeof(uint32_t);
  record_length =
      StringUtility::DeserializeStringToUint32(input.substr(pos, size));
  return true;
};
};
