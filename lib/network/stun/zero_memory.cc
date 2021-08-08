#if defined(WIN32) || defined(WIN64) || defined(__CYGWIN__)
#include <windows.h>
#else
#include <string.h>
#endif

#include "zero_memory.h"

namespace MSF {

// Code and comment taken from "OPENSSL_cleanse" of BoringSSL.
void ExplicitZeroMemory(void* ptr, size_t len) {
  assert(ptr || !len);
#if defined(WIN32) || defined(WIN64) || defined(__MINGW32__)
  SecureZeroMemory(ptr, len);
#else
  memset(ptr, 0, len);
#if !defined(__pnacl__)
  /* As best as we can tell, this is sufficient to break any optimisations that
     might try to eliminate "superfluous" memsets. If there's an easy way to
     detect memset_s, it would be better to use that. */
  __asm__ __volatile__("" : : "r"(ptr) : "memory");  // NOLINT
#endif
#endif
}

}  // namespace rtc
