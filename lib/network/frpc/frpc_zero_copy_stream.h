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
#ifndef FRPC_ZERO_COPY_STREAM_H_
#define FRPC_ZERO_COPY_STREAM_H_

#include <google/protobuf/io/zero_copy_stream.h>

namespace MSF {

class FRPCOutputStream : public google::protobuf::io::ZeroCopyOutputStream {
 public:
  FRPCOutputStream(RPCBuffer *buf, size_t size);
  bool Next(void **data, int *size) override;
  void BackUp(int count) override;
  int64_t ByteCount() const override;

 private:
  RPCBuffer *buf;
  size_t size;
};

class RPCInputStream : public google::protobuf::io::ZeroCopyInputStream {
 public:
  RPCInputStream(RPCBuffer *buf);
  bool Next(const void **data, int *size) override;
  void BackUp(int count) override;
  bool Skip(int count) override;
  int64_t ByteCount() const override;

 private:
  RPCBuffer *buf;
};

inline FRPCOutputStream::FRPCOutputStream(RPCBuffer *buf, size_t size) {
  this->buf = buf;
  this->size = size;
}

inline bool FRPCOutputStream::Next(void **data, int *size) {
  size_t tmp;

  if (this->size > 0) {
    tmp = this->size;
    if (this->buf->acquire(data, &tmp)) {
      this->size -= tmp;
      *size = (int)tmp;
      return true;
    }
  } else {
    tmp = this->buf->acquire(data);
    if (tmp > 0) {
      *size = (int)tmp;
      return true;
    }
  }

  return false;
}

inline void FRPCOutputStream::BackUp(int count) { this->buf->backup(count); }

inline int64_t FRPCOutputStream::ByteCount() const { return this->buf->size(); }

inline RPCInputStream::RPCInputStream(RPCBuffer *buf) { this->buf = buf; }

inline bool RPCInputStream::Next(const void **data, int *size) {
  size_t len = this->buf->fetch(data);

  if (len == 0) return false;

  *size = (int)len;
  return true;
}

inline bool RPCInputStream::Skip(int count) {
  return this->buf->seek(count) == count ? true : false;
}

inline void RPCInputStream::BackUp(int count) { this->buf->seek(0 - count); }

inline int64_t RPCInputStream::ByteCount() const {
  return (int64_t) this->buf->size();
}

}  // namespace srpc

#endif
