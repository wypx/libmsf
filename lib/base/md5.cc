
#include "md5.h"

#include "byteoder.h"

using namespace MSF;

/* MD5 Transforms */
#define F1(x, y, z) (z ^ (x &(y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

/* Macro for applying transforms */
#define MD5STEP(f, w, x, y, z, i, k, s) \
  (w += f(x, y, z) + i + k, w = (w << s | w >> (32 - s)) + x)

static void __md5_transform(uint32_t hash[MD5_DIGEST_WORDS],
                            const uint32_t in[MD5_BLOCK_WORDS]) {
  /*register*/ uint32_t a, b, c, d;

  a = hash[0];
  b = hash[1];
  c = hash[2];
  d = hash[3];

  MD5STEP(F1, a, b, c, d, in[0], 0xD76AA478UL, 7);
  MD5STEP(F1, d, a, b, c, in[1], 0xE8C7B756UL, 12);
  MD5STEP(F1, c, d, a, b, in[2], 0x242070DBUL, 17);
  MD5STEP(F1, b, c, d, a, in[3], 0xC1BDCEEEUL, 22);
  MD5STEP(F1, a, b, c, d, in[4], 0xF57C0FAFUL, 7);
  MD5STEP(F1, d, a, b, c, in[5], 0x4787C62AUL, 12);
  MD5STEP(F1, c, d, a, b, in[6], 0xA8304613UL, 17);
  MD5STEP(F1, b, c, d, a, in[7], 0xFD469501UL, 22);
  MD5STEP(F1, a, b, c, d, in[8], 0x698098D8UL, 7);
  MD5STEP(F1, d, a, b, c, in[9], 0x8B44F7AFUL, 12);
  MD5STEP(F1, c, d, a, b, in[10], 0xFFFF5BB1UL, 17);
  MD5STEP(F1, b, c, d, a, in[11], 0x895CD7BEUL, 22);
  MD5STEP(F1, a, b, c, d, in[12], 0x6B901122UL, 7);
  MD5STEP(F1, d, a, b, c, in[13], 0xFD987193UL, 12);
  MD5STEP(F1, c, d, a, b, in[14], 0xA679438EUL, 17);
  MD5STEP(F1, b, c, d, a, in[15], 0x49B40821UL, 22);

  MD5STEP(F2, a, b, c, d, in[1], 0xF61E2562UL, 5);
  MD5STEP(F2, d, a, b, c, in[6], 0xC040B340UL, 9);
  MD5STEP(F2, c, d, a, b, in[11], 0x265E5A51UL, 14);
  MD5STEP(F2, b, c, d, a, in[0], 0xE9B6C7AAUL, 20);
  MD5STEP(F2, a, b, c, d, in[5], 0xD62F105DUL, 5);
  MD5STEP(F2, d, a, b, c, in[10], 0x02441453UL, 9);
  MD5STEP(F2, c, d, a, b, in[15], 0xD8A1E681UL, 14);
  MD5STEP(F2, b, c, d, a, in[4], 0xE7D3FBC8UL, 20);
  MD5STEP(F2, a, b, c, d, in[9], 0x21E1CDE6UL, 5);
  MD5STEP(F2, d, a, b, c, in[14], 0xC33707D6UL, 9);
  MD5STEP(F2, c, d, a, b, in[3], 0xF4D50D87UL, 14);
  MD5STEP(F2, b, c, d, a, in[8], 0x455A14EDUL, 20);
  MD5STEP(F2, a, b, c, d, in[13], 0xA9E3E905UL, 5);
  MD5STEP(F2, d, a, b, c, in[2], 0xFCEFA3F8UL, 9);
  MD5STEP(F2, c, d, a, b, in[7], 0x676F02D9UL, 14);
  MD5STEP(F2, b, c, d, a, in[12], 0x8D2A4C8AUL, 20);

  MD5STEP(F3, a, b, c, d, in[5], 0xFFFA3942UL, 4);
  MD5STEP(F3, d, a, b, c, in[8], 0x8771F681UL, 11);
  MD5STEP(F3, c, d, a, b, in[11], 0x6D9D6122UL, 16);
  MD5STEP(F3, b, c, d, a, in[14], 0xFDE5380CUL, 23);
  MD5STEP(F3, a, b, c, d, in[1], 0xA4BEEA44UL, 4);
  MD5STEP(F3, d, a, b, c, in[4], 0x4BDECFA9UL, 11);
  MD5STEP(F3, c, d, a, b, in[7], 0xF6BB4B60UL, 16);
  MD5STEP(F3, b, c, d, a, in[10], 0xBEBFBC70UL, 23);
  MD5STEP(F3, a, b, c, d, in[13], 0x289B7EC6UL, 4);
  MD5STEP(F3, d, a, b, c, in[0], 0xEAA127FAUL, 11);
  MD5STEP(F3, c, d, a, b, in[3], 0xD4EF3085UL, 16);
  MD5STEP(F3, b, c, d, a, in[6], 0x04881D05UL, 23);
  MD5STEP(F3, a, b, c, d, in[9], 0xD9D4D039UL, 4);
  MD5STEP(F3, d, a, b, c, in[12], 0xE6DB99E5UL, 11);
  MD5STEP(F3, c, d, a, b, in[15], 0x1FA27CF8UL, 16);
  MD5STEP(F3, b, c, d, a, in[2], 0xC4AC5665UL, 23);

  MD5STEP(F4, a, b, c, d, in[0], 0xF4292244UL, 6);
  MD5STEP(F4, d, a, b, c, in[7], 0x432AFF97UL, 10);
  MD5STEP(F4, c, d, a, b, in[14], 0xAB9423A7UL, 15);
  MD5STEP(F4, b, c, d, a, in[5], 0xFC93A039UL, 21);
  MD5STEP(F4, a, b, c, d, in[12], 0x655B59C3UL, 6);
  MD5STEP(F4, d, a, b, c, in[3], 0x8F0CCC92UL, 10);
  MD5STEP(F4, c, d, a, b, in[10], 0xFFEFF47DUL, 15);
  MD5STEP(F4, b, c, d, a, in[1], 0x85845DD1UL, 21);
  MD5STEP(F4, a, b, c, d, in[8], 0x6FA87E4FUL, 6);
  MD5STEP(F4, d, a, b, c, in[15], 0xFE2CE6E0UL, 10);
  MD5STEP(F4, c, d, a, b, in[6], 0xA3014314UL, 15);
  MD5STEP(F4, b, c, d, a, in[13], 0x4E0811A1UL, 21);
  MD5STEP(F4, a, b, c, d, in[4], 0xF7537E82UL, 6);
  MD5STEP(F4, d, a, b, c, in[11], 0xBD3AF235UL, 10);
  MD5STEP(F4, c, d, a, b, in[2], 0x2AD7D2BBUL, 15);
  MD5STEP(F4, b, c, d, a, in[9], 0xEB86D391UL, 21);

  hash[0] += a;
  hash[1] += b;
  hash[2] += c;
  hash[3] += d;
}

static inline void md5_transform(uint32_t digest[MD5_DIGEST_WORDS],
                                 uint32_t block[MD5_BLOCK_WORDS]) {
  int i;

  for (i = 0; i < MD5_BLOCK_WORDS; i++) block[i] = cpu_to_le32(block[i]);

  __md5_transform(digest, block);
}

void md5_init(struct md5_ctx *ctx) {
  ctx->digest[0] = 0x67452301UL;
  ctx->digest[1] = 0xEFCDAB89UL;
  ctx->digest[2] = 0x98BADCFEUL;
  ctx->digest[3] = 0x10325476UL;

  ctx->count = 0;
}

void md5_update(struct md5_ctx *ctx, const void *data_in, size_t len) {
  const size_t offset = ctx->count & 0x3f;
  const size_t avail = MD5_BLOCK_BYTES - offset;
  const uint8_t *data = (uint8_t *)data_in;

  ctx->count += len;

  if (avail > len) {
    memcpy((uint8_t *)ctx->block + offset, data, len);
    return;
  }

  memcpy((uint8_t *)ctx->block + offset, data, avail);
  md5_transform(ctx->digest, ctx->block);
  data += avail;
  len -= avail;

  while (len >= MD5_BLOCK_BYTES) {
    memcpy(ctx->block, data, MD5_BLOCK_BYTES);
    md5_transform(ctx->digest, ctx->block);
    data += MD5_BLOCK_BYTES;
    len -= MD5_BLOCK_BYTES;
  }

  if (len) memcpy(ctx->block, data, len);
}

void md5_final(struct md5_ctx *ctx, uint8_t *out) {
  const size_t offset = ctx->count & 0x3f;
  char *p = (char *)ctx->block + offset;
  int padding = (MD5_BLOCK_BYTES - MD5_COUNTER_BYTES) - (offset + 1);
  int i;

  *p++ = 0x80;

  if (padding < 0) {
    memset(p, 0, (padding + MD5_COUNTER_BYTES));
    md5_transform(ctx->digest, ctx->block);
    p = (char *)ctx->block;
    padding = (MD5_BLOCK_BYTES - MD5_COUNTER_BYTES);
  }

  memset(p, 0, padding);

  /* 64-bit bit counter stored in LSW/LSB format */
  ctx->block[14] = cpu_to_le32(ctx->count << 3);
  ctx->block[15] = cpu_to_le32(ctx->count >> 29);

  md5_transform(ctx->digest, ctx->block);

  for (i = 0; i < MD5_DIGEST_WORDS; i++)
    ctx->digest[i] = le32_to_cpu(ctx->digest[i]);

  memcpy(out, ctx->digest, MD5_DIGEST_BYTES);
  memset(ctx, 0, sizeof(*ctx));
}
