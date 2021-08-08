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
#include <fraft/data_node_header.h>
#include <fraft/string_utility.h>

namespace MSF {
bool DataNodeHeader::Serialize(std::string *output) const {
  output->append(StringUtility::SerializeUint32ToString(node_size));
  return true;
}

bool DataNodeHeader::Deserialize(const std::string &input) {
  if (input.size() < DATA_NODE_HEADER_SIZE) {
    return false;
  }
  uint32_t size = sizeof(uint32_t);
  node_size = StringUtility::DeserializeStringToUint32(input.substr(0, size));
  return true;
}
};
