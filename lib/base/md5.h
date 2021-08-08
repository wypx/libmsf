#ifndef BASE_MD5_H_
#define BASE_MD5_H_

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

struct MD5Context {
  uint32_t buf[4];
  uint32_t bits[2];
  uint32_t in[16];
};

void MD5_init(MD5Context *context);
void MD5_update(MD5Context *context, const uint8_t *data, size_t len);
void MD5_final(MD5Context *context, uint8_t digest[16]);
void MD5_transform(uint32_t buf[4], const uint32_t in[16]);

#endif /* !MD5_H */
