
#ifndef LIB_MD5_DIGEST_H_
#define LIB_MD5_DIGEST_H_

#include "md5.h"
#include "message_digest.h"

namespace MSF {

// A simple wrapper for our MD5 implementation.
class Md5Digest : public MessageDigest {
 public:
  enum {
    k_size = 16
  };
  Md5Digest() { MD5_init(&_ctx); }
  size_t Size() const override;
  void Update(const void* buf, size_t len) override;
  size_t Finish(void* buf, size_t len) override;

 private:
  MD5Context _ctx;
};

}  // namespace MSF

#endif  // LIB_MD5_DIGEST_H_
