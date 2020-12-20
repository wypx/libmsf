
#ifndef BASE_SHA1_H_
#define BASE_SHA1_H_

#include <stdint.h>
#include <string.h>

#define SHA1_DIGEST_WORDS 5
#define SHA1_DIGEST_BYTES (SHA1_DIGEST_WORDS * 4)
#define SHA1_BLOCK_WORDS 16
#define SHA1_BLOCK_BYTES (SHA1_BLOCK_WORDS * 4)
#define SHA1_WORKSPACE_WORDS 80
#define SHA1_COUNTER_BYTES 8

struct sha1_ctx {
  uint32_t digest[SHA1_DIGEST_WORDS];
  uint32_t block[SHA1_BLOCK_WORDS];
  uint64_t count;
};

void sha1_init(struct sha1_ctx *ctx);
void sha1_update(struct sha1_ctx *ctx, const void *data_in, size_t len);
void sha1_final(struct sha1_ctx *ctx, uint8_t *out);

#endif
