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
#ifndef FRAFT_SERIALIZE_UTILITY_H_
#define FRAFT_SERIALIZE_UTILITY_H_

#include <map>
#include <fraft/proto/transaction.pb.h>

namespace MSF {

class DataTree;
bool SerializeSessionList(const std::map<uint64_t, uint64_t> &session_timeouts,
                          fraft::SessionList *session_list);
bool DeserializeSessionList(const fraft::SessionList &session_list,
                            std::map<uint64_t, uint64_t> *session_timeouts);
bool DeserializeSnapLog(const std::string &input, DataTree *data_tree,
                        std::map<uint64_t, uint64_t> *session_timeouts);
};
#endif  // FRAFT_SERIALIZE_UTILITY_H_
