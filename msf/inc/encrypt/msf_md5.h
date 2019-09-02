#ifndef __MD5_H__
#define __MD5_H__

#include "msf_utils.h"
#include <string.h>

#define MD5_BLOCK_WORDS		16
#define MD5_BLOCK_BYTES		(MD5_BLOCK_WORDS * 4)
#define MD5_DIGEST_WORDS	4
#define MD5_DIGEST_BYTES	(MD5_DIGEST_WORDS * 4)
#define MD5_COUNTER_BYTES	8

struct md5_ctx {
    u32 block[MD5_BLOCK_WORDS];
    u32 digest[MD5_DIGEST_WORDS];
    u64 count;
};

void md5_init(struct md5_ctx *ctx);
void md5_update(struct md5_ctx *ctx, const void *data_in, size_t len);
void md5_final(struct md5_ctx *ctx, u8 *out);

#endif /* !MD5_H */

