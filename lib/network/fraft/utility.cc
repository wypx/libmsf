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
#include <map>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <base/logging.h>
#include <fraft/utility.h>

namespace MSF {
bool SortFiles(const std::string &directory, const std::string &prefix,
               bool ascending, std::list<std::string> *files) {
  DIR *dir = NULL;
  struct dirent *dirp = NULL;
  dir = opendir(directory.c_str());
  if (dir == NULL) {
    LOG(ERROR) << "opendir " << directory << " error:" << strerror(errno);
    return false;
  }
  uint64_t gxid = 0;
  std::map<uint64_t, std::string, std::greater<uint64_t> > descending_map;
  std::map<uint64_t, std::string, std::less<uint64_t> > ascending_map;
  while ((dirp = readdir(dir)) != NULL) {
    if (strncmp(dirp->d_name, prefix.c_str(), prefix.length())) {
      continue;
    }
    // remember to skip '.'
    gxid = atol(dirp->d_name + prefix.length() + 1);
    if (ascending) {
      ascending_map[gxid] = dirp->d_name;
    } else {
      descending_map[gxid] = dirp->d_name;
    }
  }
  closedir(dir);
  if (ascending) {
    std::map<uint64_t, std::string, std::greater<uint64_t> >::iterator iter;
    for (iter = ascending_map.begin(); iter != ascending_map.end(); ++iter) {
      files->push_back(iter->second);
    }
  } else {
    std::map<uint64_t, std::string, std::less<uint64_t> >::iterator iter;
    for (iter = descending_map.begin(); iter != descending_map.end(); ++iter) {
      files->push_back(iter->second);
    }
  }
  return !files->empty();
}

uint64_t GetGxidOfFileName(const std::string &file_name,
                           const std::string &prefix) {
  // remember to skip '.'
  return ::atol(file_name.c_str() + prefix.length() + 1);
}
};
