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
#include <fraft/data_tree.h>
#include <fraft/transaction_log_header.h>
#include <fraft/serialize_utility.h>

namespace MSF {

bool SerializeSessionList(const std::map<uint64_t, uint64_t> &session_timeouts,
                          fraft::SessionList *session_list) {
  std::map<uint64_t, uint64_t>::const_iterator iter = session_timeouts.begin();
  for (; iter != session_timeouts.end(); ++iter) {
    fraft::Session *session = session_list->add_sessions();
    session->set_id(iter->first);
    session->set_timeout(iter->second);
  }
  return true;
}

bool DeserializeSessionList(const fraft::SessionList &session_list,
                            std::map<uint64_t, uint64_t> *session_timeouts) {
  for (int i = 0; i < session_list.sessions_size(); ++i) {
    fraft::Session session = session_list.sessions(i);
    (*session_timeouts)[session.id()] = session.timeout();
  }
  return true;
}

bool DeserializeSnapLog(const std::string &input, DataTree *data_tree,
                        std::map<uint64_t, uint64_t> *session_timeouts) {
  uint32_t pos = 0, size = 0;
  SnapLogFileHeader header;
  header.Deserialize(input);

  pos += SNAP_LOG_HEADER_SIZE;
  size = header.session_size;
  fraft::SessionList session_list;
  session_list.ParseFromString(input.substr(pos, size));
  DeserializeSessionList(session_list, session_timeouts);
  pos += size;
  data_tree->Deserialize(input.substr(pos));
  return true;
}
};
