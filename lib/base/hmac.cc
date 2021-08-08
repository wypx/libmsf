#include "hmac.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if defined(HAS_OPENSSL)
#include <openssl/evp.h>
#include <openssl/hmac.h>

bool CryptoHmac(const uint8_t* key, size_t key_len,
                const CryptoHmacMsg messages[], size_t num_messages,
                uint8_t* truncated_digest, size_t truncated_digest_len) {
  HMAC_CTX context = {0};
  HMAC_CTX_init(&context);
  if (!HMAC_Init(&context, key, key_len, EVP_sha256())) return false;

  for (size_t i = 0; i < num_messages; ++i) {
    if (messages[i].num_bytes &&
        (!messages[i].bytes ||
         !HMAC_Update(&context, messages[i].bytes, messages[i].num_bytes))) {
      return false;
    }
  }

  const size_t kFullDigestLen = (size_t)EVP_MD_size(EVP_sha256());
  if (truncated_digest_len > kFullDigestLen) {
    return false;
  }

  uint8_t digest[kFullDigestLen];
  uint32_t len = kFullDigestLen;

  bool result = HMAC_Final(&context, digest, &len) && kFullDigestLen == len;
  HMAC_CTX_cleanup(&context);
  if (result) {
    memcpy(truncated_digest, digest, truncated_digest_len);
  }
  return result;
}
#endif