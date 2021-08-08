
#include "md5_digest.h"

namespace MSF {

size_t Md5Digest::Size() const { return k_size; }

void Md5Digest::Update(const void* buf, size_t len) {
  MD5_update(&_ctx, static_cast<const uint8_t*>(buf), len);
}

size_t Md5Digest::Finish(void* buf, size_t len) {
  if (len < k_size) {
    return 0;
  }
  MD5_final(&_ctx, static_cast<uint8_t*>(buf));
  MD5_init(&_ctx);  // Reset for next use.
  return k_size;
}

}  // namespace MSF
