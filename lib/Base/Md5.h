#ifndef __MD5_H__
#define __MD5_H__

#include <stdint.h>
#include <string.h>

#define MD5_BLOCK_WORDS 16
#define MD5_BLOCK_BYTES (MD5_BLOCK_WORDS * 4)
#define MD5_DIGEST_WORDS 4
#define MD5_DIGEST_BYTES (MD5_DIGEST_WORDS * 4)
#define MD5_COUNTER_BYTES 8

struct md5_ctx {
  uint32_t block[MD5_BLOCK_WORDS];
  uint32_t digest[MD5_DIGEST_WORDS];
  uint64_t count;
};

void md5_init(struct md5_ctx *ctx);
void md5_update(struct md5_ctx *ctx, const void *data_in, size_t len);
void md5_final(struct md5_ctx *ctx, uint8_t *out);

#endif /* !MD5_H */
