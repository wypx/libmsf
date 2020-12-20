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
// #include <uuid/uuid.h>
#include "utils.h"

namespace MSF {

// std::string NewUuid() {
//   uuid_t sUuid;
//   char szTemp[64];
//   uuid_generate(sUuid);
//   uuid_unparse(sUuid, szTemp);
//   return szTemp;
// }

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

std::string exePath() {
  char buffer[PATH_MAX * 2 + 1] = {0};
  int n = -1;
#if defined(_WIN32)
  n = GetModuleFileNameA(NULL, buffer, sizeof(buffer));
#elif defined(__MACH__) || defined(__APPLE__)
  n = sizeof(buffer);
  if (uv_exepath(buffer, &n) != 0) {
    n = -1;
  }
#elif defined(__linux__)
  n = readlink("/proc/self/exe", buffer, sizeof(buffer));
#endif

  std::string filePath;
  if (n <= 0) {
    filePath = "./";
  } else {
    filePath = buffer;
  }

#if defined(_WIN32)
  // windows下把路径统一转换层unix风格，因为后续都是按照unix风格处理的
  for (auto &ch : filePath) {
    if (ch == '\\') {
      ch = '/';
    }
  }
#endif  // defined(_WIN32)

  return filePath;
}

std::string exeDir() {
  std::string path = exePath();
  return path.substr(0, path.rfind('/') + 1);
}
std::string exeName() {
  std::string path = exePath();
  return path.substr(path.rfind('/') + 1);
}
// // string转小写
// std::string &strToLower(std::string &str) {
//     std::transform(str.begin(), str.end(), str.begin(), std::towlower);
//     return str;
// }
// // string转大写
// std::string &strToUpper(std::string &str) {
//     std::transform(str.begin(), str.end(), str.begin(), std::towupper);
//     return str;
// }

// // string转小写
// std::string strToLower(std::string &&str) {
//     std::transform(str.begin(), str.end(), str.begin(), std::towlower);
//     return std::move(str);
// }
// // string转大写
// std::string strToUpper(std::string &&str) {
//     std::transform(str.begin(), str.end(), str.begin(), std::towupper);
//     return std::move(str);
// }

std::vector<std::string> StringSplit(const std::string &s,
                                     const std::string &delim = " ") {
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

void replace(std::string &str, const std::string &old_str,
             const std::string &new_str) {
  if (old_str.empty() || old_str == new_str) {
    return;
  }
  auto pos = str.find(old_str);
  if (pos == std::string::npos) {
    return;
  }
  str.replace(pos, old_str.size(), new_str);
  replace(str, old_str, new_str);
}

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

}  // namespace MSF