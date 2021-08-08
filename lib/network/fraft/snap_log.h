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
#ifndef FRAFT_SNAP_LOG_H_
#define FRAFT_SNAP_LOG_H_

#include <map>
#include <fraft/data_tree.h>

namespace MSF {

class SnapLog {
 public:
  explicit SnapLog(const std::string &log_dir);
  ~SnapLog();

  bool Deserialize(DataTree *data_tree,
                   std::map<uint64_t, uint64_t> *session_timeouts) const;
  bool Serialize(const DataTree &data_tree,
                 const std::map<uint64_t, uint64_t> &session_timeouts,
                 std::string *output);

 private:
  struct Impl;
  Impl *impl_;
};
};
#endif  // FRAFT_SNAP_LOG_H_
