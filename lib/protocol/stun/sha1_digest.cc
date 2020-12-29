
#include "sha1_digest.h"

namespace MSF {

size_t Sha1Digest::Size() const { return k_size; }

void Sha1Digest::Update(const void* buf, size_t len) {
  SHA1_update(&_ctx, static_cast<const uint8_t*>(buf), len);
}

size_t Sha1Digest::Finish(void* buf, size_t len) {
  if (len < k_size) {
    return 0;
  }
  SHA1_final(&_ctx, static_cast<uint8_t*>(buf));
  SHA1_init(&_ctx);  // Reset for next use.
  return k_size;
}

}  // namespace rtcbase
