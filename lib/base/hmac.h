#ifndef BASE_HMAC_H_
#define BASE_HMAC_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  const uint8_t* bytes;
  size_t num_bytes;
} CryptoHmacMsg;

/**
 * Compute HMAC over a list of messages, which is equivalent to computing HMAC
 * over the concatenation of all the messages. The HMAC output will be truncated
 * to the desired length truncated_digest_len, and written into trucated_digest.
 */
bool CryptoHmac(const uint8_t* key, size_t key_len,
                const CryptoHmacMsg messages[], size_t num_messages,
                uint8_t* truncated_digest, size_t truncated_digest_len);

#endif
