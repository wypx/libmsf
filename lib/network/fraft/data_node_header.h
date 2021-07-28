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
#ifndef FRAFT_DATA_NODE_HEADER_H_
#define FRAFT_DATA_NODE_HEADER_H_

#include <stdint.h>
#include <string>

namespace MSF {

struct DataNodeHeader {
  uint32_t node_size;
  DataNodeHeader() : node_size(0) {}
  bool Serialize(std::string *output) const;
  bool Deserialize(const std::string &input);
};
#define DATA_NODE_HEADER_SIZE sizeof(DataNodeHeader)
};
#endif  // FRAFT_DATA_NODE_HEADER_H_
