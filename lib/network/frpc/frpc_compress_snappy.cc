/**************************************************************************
 *
 * Copyright (c) 2017-2021, luotang.me <wypx520@gmail.com>, China.
 * All rights reserved.
 *
 * Distributed under the terms of the GNU General Public License v2.
 *
 * This software is provided 'as is' with no explicit or implied warranties
 * in respect of its properties, including, but not limited to, correctness
 * and/or fitness for purpose.
 *
 **************************************************************************/
#include <snappy.h>
#include <snappy-sinksource.h>
#include "frpc_compress_snappy.h"

#if 0

namespace MSF {

class RPCSnappySink : public snappy::Sink {
 public:
  RPCSnappySink(Buffer *buf) { this->buf = buf; }

  void Append(const char *bytes, size_t n) override {
    this->buf->append(bytes, n, BUFFER_MODE_COPY);
  }

  size_t size() const { return this->buf->size(); }

 private:
  Buffer *buf;
};

class RPCSnappySource : public snappy::Source {
 public:
  RPCSnappySource(Buffer *buf) {
    this->buf = buf;
    this->buf_size = this->buf->size();
    this->pos = 0;
  }

  size_t Available() const override { return this->buf_size - this->pos; }

  const char *Peek(size_t *len) override {
    const void *pos;

    *len = this->buf->peek(&pos);
    return (const char *)pos;
  }

  void Skip(size_t n) override { this->pos += this->buf->seek(n); }

 private:
  Buffer *buf;
  size_t buf_size;
  size_t pos;
};

int SnappyManager::SnappyCompress(const char *msg, size_t msglen, char *buf,
                                  size_t buflen) {
  size_t compressed_len = buflen;

  snappy::RawCompress(msg, msglen, buf, &compressed_len);
  if (compressed_len > buflen) return -1;

  return (int)compressed_len;
}

int SnappyManager::SnappyDecompress(const char *buf, size_t buflen, char *msg,
                                    size_t msglen) {
  size_t origin_len;

  if (snappy::GetUncompressedLength(buf, buflen, &origin_len) &&
      origin_len <= msglen && snappy::RawUncompress(buf, buflen, msg)) {
    return (int)origin_len;
  }

  return -1;
}

int SnappyManager::SnappyCompressIOVec(Buffer *src, Buffer *dst) {
  RPCSnappySource source(src);
  RPCSnappySink sink(dst);

  return (int)snappy::Compress(&source, &sink);
}

int SnappyManager::SnappyDecompressIOVec(Buffer *src, Buffer *dst) {
  RPCSnappySource source(src);
  RPCSnappySink sink(dst);

  //	if (snappy::IsValidCompressed(&source))
  //	if (snappy::GetUncompressedLength(&source, &origin_len))
  return (int)snappy::Uncompress(&source, &sink) ? sink.size() : -1;
}

int SnappyManager::SnappyLeaseSize(size_t origin_size) {
  return (int)snappy::MaxCompressedLength(origin_size);
}

}  // end namespace MSF

#endif