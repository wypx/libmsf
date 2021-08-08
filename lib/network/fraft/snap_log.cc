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
#include <stdio.h>
#include <base/logging.h>
#include <base/file.h>

#include <fraft/snap_log.h>
#include <fraft/transaction_log_header.h>
#include <fraft/utility.h>
#include <fraft/serialize_utility.h>

namespace {
static const char kSnapLogFileHeaderMagic[] = "GSNLOG";
static const uint32_t kLogVersion = 1;
static const uint64_t kDbId = 1;
};

namespace MSF {
struct SnapLog::Impl {
 public:
  explicit Impl(const std::string &log_dir);
  ~Impl();

  bool Deserialize(DataTree *data_tree,
                   std::map<uint64_t, uint64_t> *session_timeouts) const;
  bool Serialize(const DataTree &data_tree,
                 const std::map<uint64_t, uint64_t> &session_timeouts,
                 std::string *output);
  bool FindNValidSnapshots(uint32_t count, std::list<std::string> *files) const;

  std::string log_dir_;
};

SnapLog::Impl::Impl(const std::string &log_dir) : log_dir_(log_dir) {}

SnapLog::Impl::~Impl() {}

bool SnapLog::Impl::Deserialize(
    DataTree *data_tree, std::map<uint64_t, uint64_t> *session_timeouts) const {
  std::list<std::string> files;
  if (!FindNValidSnapshots(100, &files)) {
    return false;
  }
  std::list<std::string>::iterator iter = files.begin();
  for (; iter != files.end(); ++iter) {
    std::string content;
    ReadFileContents(*iter, &content);
    SnapLogFileHeader header;
    header.Deserialize(content);
    fraft::SessionList sessions;
    content = content.substr(SNAP_LOG_HEADER_SIZE);

    sessions.ParseFromString(content.substr(0, header.session_size));
    DeserializeSessionList(sessions, session_timeouts);
    content = content.substr(header.session_size);
    data_tree->Deserialize(content);
  }
  return true;
}

bool SnapLog::Impl::Serialize(
    const DataTree &data_tree,
    const std::map<uint64_t, uint64_t> &session_timeouts, std::string *output) {
  SnapLogFileHeader header;
  header.magic = atol(kSnapLogFileHeaderMagic);
  header.version = kLogVersion;
  header.dbid = kDbId;
  fraft::SessionList session_list;
  SerializeSessionList(session_timeouts, &session_list);
  header.session_size = session_list.ByteSizeLong();
  std::string header_content;
  header.Serialize(&header_content);
  std::string session_list_content;
  session_list.SerializeToString(&session_list_content);
  std::string data_tree_content;
  data_tree.Serialize(&data_tree_content);
  *output = header_content;
  output->append(session_list_content);
  output->append(data_tree_content);
  return true;
}

bool SnapLog::Impl::FindNValidSnapshots(uint32_t count,
                                        std::list<std::string> *files) const {
  if (!SortFiles(log_dir_, "snap", false, files)) {
    return false;
  }
  return true;
}

SnapLog::SnapLog(const std::string &log_dir) : impl_(new Impl(log_dir)) {}

SnapLog::~SnapLog() { delete impl_; }

bool SnapLog::Deserialize(
    DataTree *data_tree, std::map<uint64_t, uint64_t> *session_timeouts) const {
  return impl_->Deserialize(data_tree, session_timeouts);
}

bool SnapLog::Serialize(const DataTree &data_tree,
                        const std::map<uint64_t, uint64_t> &session_timeouts,
                        std::string *output) {
  return impl_->Serialize(data_tree, session_timeouts, output);
}
};
