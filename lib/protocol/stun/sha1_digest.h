
#ifndef __RTCBASE_SHA1_DIGEST_H_
#define __RTCBASE_SHA1_DIGEST_H_

#include "message_digest.h"
#include "sha1.h"

namespace MSF {

// A simple wrapper for our SHA-1 implementation.
class Sha1Digest : public MessageDigest {
 public:
  enum {
    k_size = SHA1_DIGEST_SIZE
  };
  Sha1Digest() { SHA1_init(&_ctx); }
  size_t Size() const override;
  void Update(const void* buf, size_t len) override;
  size_t Finish(void* buf, size_t len) override;

 private:
  SHA1_CTX _ctx;
};

}  // namespace rtcbase

#endif  //__RTCBASE_SHA1_DIGEST_H_
