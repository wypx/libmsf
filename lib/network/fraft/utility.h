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
#ifndef FRAFT_UTILITY_H_
#define FRAFT_UTILITY_H_

#include <list>
#include <string>

namespace MSF {

bool SortFiles(const std::string &directory, const std::string &prefix,
               bool ascending, std::list<std::string> *files);
uint64_t GetGxidOfFileName(const std::string &file_name,
                           const std::string &prefix);
}
#endif  // FRAFT_UTILITY_H_
