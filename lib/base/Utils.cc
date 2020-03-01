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
#include <base/Utils.h>

namespace MSF {
namespace BASE {

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

#include <endian.h>
#include <netinet/in.h>
unsigned long long ntohll(unsigned long long val) {
  if (__BYTE_ORDER == __LITTLE_ENDIAN) {
    return (((unsigned long long)htonl((int)((val << 32) >> 32))) << 32) |
           (unsigned int)htonl((int)(val >> 32));
  } else if (__BYTE_ORDER == __BIG_ENDIAN) {
    return val;
  }
}

unsigned long long htonll(unsigned long long val) {
  if (__BYTE_ORDER == __LITTLE_ENDIAN) {
    return (((unsigned long long)htonl((int)((val << 32) >> 32))) << 32) |
           (unsigned int)htonl((int)(val >> 32));
  } else if (__BYTE_ORDER == __BIG_ENDIAN) {
    return val;
  }
}

}  // namespace BASE
}  // namespace MSF