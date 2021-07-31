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
#include <endian.h>
#include <netinet/in.h>

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

#include "utils.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <uuid/uuid.h>
#endif

namespace MSF {

// uuid of system
// cat /proc/sys/kernel/random/uuid
#if 1
std::string NewUuid() {
  std::string res;
#ifdef WIN32
  UUID uuid;
  ::UuidCreate(&uuid);
  char *uuid_cstr = nullptr;
  ::UuidToStringA(&uuid, reinterpret_cast<CSTR *>(&uuid_cstr));
  res = std::string(uuid_cstr);
  :StringFreeA(reinterpret_cast<CSTR *>(&uuid_cstr));
#else
  uuid_t uuid;
  char uuid_cstr[37];  // 36 byte uuid plus null.
  ::uuid_generate(uuid);
  ::uuid_unparse(uuid, uuid_cstr);
  res = std::string(uuid_cstr);
#endif

  return res;
}
#endif

static const uint32_t kMaxHostNameSize = 255;
static inline std::string GetLocalHostName() {
  char str[kMaxHostNameSize + 1];
  if (0 != gethostname(str, kMaxHostNameSize + 1)) {
    return "";
  }
  std::string hostname(str);
  return hostname;
}

static const uint32_t MAX_PATH_LENGHT = 10240;
static inline bool SplitPath(const std::string &path,
                             std::vector<std::string> *element,
                             bool *isdir = NULL) {
  if (path.empty() || path[0] != '/' || path.size() > MAX_PATH_LENGHT) {
    return false;
  }
  element->clear();
  size_t last_pos = 0;
  for (size_t i = 1; i <= path.size(); i++) {
    if (i == path.size() || path[i] == '/') {
      if (last_pos + 1 < i) {
        element->push_back(path.substr(last_pos + 1, i - last_pos - 1));
      }
      last_pos = i;
    }
  }
  if (isdir) {
    *isdir = (path[path.size() - 1] == '/');
  }
  return true;
}

static inline void EncodeBigEndian(char *buf, uint64_t value) {
  buf[0] = (value >> 56) & 0xff;
  buf[1] = (value >> 48) & 0xff;
  buf[2] = (value >> 40) & 0xff;
  buf[3] = (value >> 32) & 0xff;
  buf[4] = (value >> 24) & 0xff;
  buf[5] = (value >> 16) & 0xff;
  buf[6] = (value >> 8) & 0xff;
  buf[7] = value & 0xff;
}

static inline uint64_t DecodeBigEndian64(const char *buf) {
  return ((static_cast<uint64_t>(static_cast<uint8_t>(buf[0]))) << 56 |
          (static_cast<uint64_t>(static_cast<uint8_t>(buf[1])) << 48) |
          (static_cast<uint64_t>(static_cast<uint8_t>(buf[2])) << 40) |
          (static_cast<uint64_t>(static_cast<uint8_t>(buf[3])) << 32) |
          (static_cast<uint64_t>(static_cast<uint8_t>(buf[4])) << 24) |
          (static_cast<uint64_t>(static_cast<uint8_t>(buf[5])) << 16) |
          (static_cast<uint64_t>(static_cast<uint8_t>(buf[6])) << 8) |
          (static_cast<uint64_t>(static_cast<uint8_t>(buf[7]))));
}

static inline void EncodeBigEndian(char *buf, uint32_t value) {
  buf[0] = (value >> 24) & 0xff;
  buf[1] = (value >> 16) & 0xff;
  buf[2] = (value >> 8) & 0xff;
  buf[3] = value & 0xff;
}

static inline uint32_t DecodeBigEndian32(const char *buf) {
  return ((static_cast<uint64_t>(static_cast<uint8_t>(buf[0])) << 24) |
          (static_cast<uint64_t>(static_cast<uint8_t>(buf[1])) << 16) |
          (static_cast<uint64_t>(static_cast<uint8_t>(buf[2])) << 8) |
          (static_cast<uint64_t>(static_cast<uint8_t>(buf[3]))));
}

void msf_nsleep(int ns) {
  struct timespec req;
  req.tv_sec = 0;
  req.tv_nsec = ns;
  nanosleep(&req, 0);
}

void msf_susleep(int s, int us) {
  struct timeval tv;
  tv.tv_sec = s;
  tv.tv_usec = us;
  select(0, NULL, NULL, NULL, &tv);
}

bool msf_safe_strtoull(const char *str, uint64_t *out) {
  errno = 0;
  *out = 0;
  char *endptr;
  uint64_t ull = strtoull(str, &endptr, 10);
  if ((errno == ERANGE) || (str == endptr)) {
    return false;
  }

  if (isspace(*endptr) || (*endptr == '\0' && endptr != str)) {
    if ((long long)ull < 0) {
      /* only check for negative signs in the uncommon case when
       * the unsigned number is so big that it's negative as a
       * signed number. */
      if (strchr(str, '-') != NULL) {
        return false;
      }
    }
    *out = ull;
    return true;
  }
  return false;
}

bool msf_safe_strtoll(const char *str, uint64_t *out) {
  errno = 0;
  *out = 0;
  char *endptr;
  int64_t ll = strtoll(str, &endptr, 10);
  if ((errno == ERANGE) || (str == endptr)) {
    return false;
  }

  if (isspace(*endptr) || (*endptr == '\0' && endptr != str)) {
    *out = ll;
    return true;
  }
  return false;
}

bool msf_safe_strtoul(const char *str, uint32_t *out) {
  char *endptr = NULL;
  uint64_t l = 0;
  *out = 0;
  errno = 0;

  l = strtoul(str, &endptr, 10);
  if ((errno == ERANGE) || (str == endptr)) {
    return false;
  }

  if (isspace(*endptr) || (*endptr == '\0' && endptr != str)) {
    if ((long)l < 0) {
      /* only check for negative signs in the uncommon case when
       * the unsigned number is so big that it's negative as a
       * signed number. */
      if (strchr(str, '-') != NULL) {
        return false;
      }
    }
    *out = l;
    return true;
  }
  return false;
}

bool msf_safe_strtol(const char *str, int32_t *out) {
  errno = 0;
  *out = 0;
  char *endptr;
  long l = strtol(str, &endptr, 10);
  if ((errno == ERANGE) || (str == endptr)) {
    return false;
  }

  if (isspace(*endptr) || (*endptr == '\0' && endptr != str)) {
    *out = l;
    return true;
  }
  return false;
}

size_t strlcpy(char *dst, const char *src, size_t size) {
  size_t bytes = 0;
  char *q = dst;
  const char *p = src;
  char ch;

  while ((ch = *p++)) {
    if (bytes + 1 < size) *q++ = ch;
    bytes++;
  }

  /* If size == 0 there is no space for a final null... */
  if (size) *q = '\0';
  return bytes;
}

size_t strlcat(char *dst, const char *src, size_t size) {
  size_t bytes = 0;
  char *q = dst;
  const char *p = src;
  char ch;

  while (bytes < size && *q) {
    q++;
    bytes++;
  }
  if (bytes == size) return (bytes + strlen(src));

  while ((ch = *p++)) {
    if (bytes + 1 < size) *q++ = ch;
    bytes++;
  }

  *q = '\0';
  return bytes;
}

// string转小写
std::string &StringToLower(std::string &str) {
  std::transform(str.begin(), str.end(), str.begin(), std::towlower);
  return str;
}
std::string StringToLower(std::string &&str) {
  std::transform(str.begin(), str.end(), str.begin(), std::towlower);
  return std::move(str);
}

// string转大写
std::string &StringToUpper(std::string &str) {
  std::transform(str.begin(), str.end(), str.begin(), std::towupper);
  return str;
}
std::string StringToUpper(std::string &&str) {
  std::transform(str.begin(), str.end(), str.begin(), std::towupper);
  return std::move(str);
}

std::vector<std::string> StringSplit(const std::string &s,
                                     const std::string &delim) {
  std::vector<std::string> elems;
  size_t pos = 0;
  size_t len = s.length();
  size_t delim_len = delim.length();
  if (delim_len == 0) return elems;
  while (pos < len) {
    int find_pos = s.find(delim, pos);
    if (find_pos < 0) {
      elems.push_back(s.substr(pos, len - pos));
      break;
    }
    elems.push_back(s.substr(pos, find_pos - pos));
    pos = find_pos + delim_len;
  }
  return elems;
}

void StringReplace(std::string &str, const std::string &old_str,
                   const std::string &new_str) {
  if (old_str.empty() || old_str == new_str) {
    return;
  }
  auto pos = str.find(old_str);
  if (pos == std::string::npos) {
    return;
  }
  str.replace(pos, old_str.size(), new_str);
  StringReplace(str, old_str, new_str);
}

std::string StringPrev(const std::string &str, const std::string &sub) {
  size_t pos = str.find(sub);
  if (pos == std::string::npos) {
    return str;
  }
  return str.substr(0, pos);
}

std::string StringNext(const std::string &str, const std::string &sub) {
  size_t pos = str.find(sub);
  if (pos == std::string::npos) {
    return str;
  }
  return str.substr(pos + sub.size());
}

#if 0
#define TRIM(s, chars)                                                   \
  do {                                                                   \
    string map(0xFF, '\0');                                              \
    for (auto &ch : chars) {                                             \
      std::map[(uint8_t &)ch] = '\1';                                    \
    }                                                                    \
    while (s.size() && std::map.at((uint8_t &)s.back())) s.pop_back();   \
    while (s.size() && std::map.at((uint8_t &)s.front())) s.erase(0, 1); \
    return s;                                                            \
  } while (0);

//去除前后的空格、回车符、制表符
std::string& trim(std::string &s, const std::string &chars) {
    TRIM(s, chars);
}
std::string trim(std::string &&s, const std::string &chars) {
    TRIM(s, chars);
}
#endif

// 删除std::String类型字符串首尾空格

#if 0
// 删除左侧空格
std::string &trimleft(std::string &str) 
{ 
	str.erase(0, str.find_first_not_of(std::ctype_base::space));
	return str;
} 
 
// 删除右侧空格
std::string &trimright(std::string &str) 
{ 
	str.erase(str.find_last_not_of(std::ctype_base::space) + 1);
	return str;
} 
 
// 删除首尾空格
std::string &trim(std::string &str) 
{ 
	return trimleft(trimright(str)); 
}
#endif

// bool IsIP(const char *str){
//     return INADDR_NONE != inet_addr(str);
// }

unsigned long long ntohll(unsigned long long val) {
  if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) {
    return (((unsigned long long)htonl((int)((val << 32) >> 32))) << 32) |
           (unsigned int)htonl((int)(val >> 32));
  } else if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) {
    return val;
  }
}

unsigned long long htonll(unsigned long long val) {
  if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) {
    return (((unsigned long long)htonl((int)((val << 32) >> 32))) << 32) |
           (unsigned int)htonl((int)(val >> 32));
  } else if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) {
    return val;
  }
}

#if 0

CFSTable::CFSTable(bool mounted, const char* fsname_prefix) throw ()
{
    if (mounted)
        _fp = setmntent(_PATH_MOUNTED, "r");
    else
        _fp = setmntent(_PATH_FSTAB, "r");

    if (fsname_prefix != NULL)
        _fsname_prefix = fsname_prefix;
}

CFSTable::~CFSTable() throw ()
{
    if (_fp != NULL)
    {        
        endmntent(_fp);
        _fp = NULL;
    }
}

void CFSTable::reset() throw ()
{
    if (_fp != NULL)
        rewind(_fp);
}

CFSTable::fs_entry_t* CFSTable::get_entry(fs_entry_t& entry) throw ()
{
    struct mntent* ent = NULL;

    for (;;)
    {    
        ent = getmntent(_fp);
        if (NULL == ent) return NULL; // 找完了所有的

        if (_fsname_prefix.empty())
        {
            break;
        }
        else
        {
            // 文件系统名的前经验不匹配，则找下一个
            if (0 == strncmp(ent->mnt_fsname, _fsname_prefix.c_str(), _fsname_prefix.length()))
                break; // 找到一个
        }
    }

    entry.fs_name   = ent->mnt_fsname;
    entry.dir_path  = ent->mnt_dir;
    entry.type_name = ent->mnt_type;
    return &entry;
}
#endif

}  // namespace MSF