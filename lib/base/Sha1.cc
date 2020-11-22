
#include "Sha1.h"

#include "ByteOder.h"

using namespace MSF;

/* SHA1 transforms */
#define F1(x, y, z) (z ^ (x &(y ^ z)))      /* x ? y : z */
#define F2(x, y, z) (x ^ y ^ z)             /* XOR */
#define F3(x, y, z) ((x &y) + (z &(x ^ y))) /* majority */

/* SHA1 per-round constants */
#define K1 0x5A827999UL /* Rounds  0-19: sqrt(2) * 2^30 */
#define K2 0x6ED9EBA1UL /* Rounds 20-39: sqrt(3) * 2^30 */
#define K3 0x8F1BBCDCUL /* Rounds 40-59: sqrt(5) * 2^30 */
#define K4 0xCA62C1D6UL /* Rounds 60-79: sqrt(10) * 2^30 */

static inline uint32_t rol32(uint32_t word, uint32_t shift) {
  return (word << shift) | (word >> (32 - shift));
}

static void __sha1_transform(uint32_t hash[SHA1_DIGEST_WORDS],
                             const uint32_t in[SHA1_BLOCK_WORDS],
                             uint32_t W[SHA1_WORKSPACE_WORDS]) {
  /*register*/ uint32_t a, b, c, d, e, t;
  int i;

  for (i = 0; i < SHA1_BLOCK_WORDS; i++) W[i] = cpu_to_be32(in[i]);

  for (i = 0; i < 64; i++)
    W[i + 16] = rol32(W[i + 13] ^ W[i + 8] ^ W[i + 2] ^ W[i], 1);

  a = hash[0];
  b = hash[1];
  c = hash[2];
  d = hash[3];
  e = hash[4];

  for (i = 0; i < 20; i++) {
    t = F1(b, c, d) + K1 + rol32(a, 5) + e + W[i];
    e = d;
    d = c;
    c = rol32(b, 30);
    b = a;
    a = t;
  }

  for (; i < 40; i++) {
    t = F2(b, c, d) + K2 + rol32(a, 5) + e + W[i];
    e = d;
    d = c;
    c = rol32(b, 30);
    b = a;
    a = t;
  }

  for (; i < 60; i++) {
    t = F3(b, c, d) + K3 + rol32(a, 5) + e + W[i];
    e = d;
    d = c;
    c = rol32(b, 30);
    b = a;
    a = t;
  }

  for (; i < 80; i++) {
    t = F2(b, c, d) + K4 + rol32(a, 5) + e + W[i];
    e = d;
    d = c;
    c = rol32(b, 30);
    b = a;
    a = t;
  }

  hash[0] += a;
  hash[1] += b;
  hash[2] += c;
  hash[3] += d;
  hash[4] += e;
}

static inline void sha1_transform(uint32_t digest[SHA1_DIGEST_WORDS],
                                  const uint32_t block[SHA1_BLOCK_WORDS]) {
  uint32_t workspace[SHA1_WORKSPACE_WORDS];

  __sha1_transform(digest, block, workspace);
}

void sha1_init(struct sha1_ctx *ctx) {
  ctx->digest[0] = 0x67452301UL;
  ctx->digest[1] = 0xEFCDAB89UL;
  ctx->digest[2] = 0x98BADCFEUL;
  ctx->digest[3] = 0x10325476UL;
  ctx->digest[4] = 0xC3D2E1F0UL;

  ctx->count = 0;
}

void sha1_update(struct sha1_ctx *ctx, const void *data_in, size_t len) {
  const size_t offset = ctx->count & 0x3f;
  const size_t avail = SHA1_BLOCK_BYTES - offset;
  const uint8_t *data = static_cast<const uint8_t *>(data_in);

  ctx->count += len;

  if (avail > len) {
    memcpy((uint8_t *)ctx->block + offset, data, len);
    return;
  }

  memcpy((uint8_t *)ctx->block + offset, data, avail);
  sha1_transform(ctx->digest, ctx->block);
  data += avail;
  len -= avail;

  while (len >= SHA1_BLOCK_BYTES) {
    memcpy(ctx->block, data, SHA1_BLOCK_BYTES);
    sha1_transform(ctx->digest, ctx->block);
    data += SHA1_BLOCK_BYTES;
    len -= SHA1_BLOCK_BYTES;
  }

  if (len) memcpy(ctx->block, data, len);
}

void sha1_final(struct sha1_ctx *ctx, uint8_t *out) {
  const size_t offset = ctx->count & 0x3f;
  char *p = (char *)ctx->block + offset;
  int padding = (SHA1_BLOCK_BYTES - SHA1_COUNTER_BYTES) - (offset + 1);
  int i;

  *p++ = 0x80;

  if (padding < 0) {
    memset(p, 0, (padding + SHA1_COUNTER_BYTES));
    sha1_transform(ctx->digest, ctx->block);
    p = (char *)ctx->block;
    padding = (SHA1_BLOCK_BYTES - SHA1_COUNTER_BYTES);
  }

  memset(p, 0, padding);

  /* 64-bit bit counter stored in MSW/MSB format */
  ctx->block[14] = cpu_to_be32(ctx->count >> 29);
  ctx->block[15] = cpu_to_be32(ctx->count << 3);

  sha1_transform(ctx->digest, ctx->block);

  for (i = 0; i < SHA1_DIGEST_WORDS; i++)
    ctx->digest[i] = be32_to_cpu(ctx->digest[i]);

  memcpy(out, ctx->digest, SHA1_DIGEST_BYTES);
  memset(ctx, 0, sizeof(*ctx));
}
