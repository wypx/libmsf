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
#ifndef FRPC_COMPRESS_H_
#define FRPC_COMPRESS_H_

#include <base/buffer.h>

#include "frpc.pb.h"

namespace MSF {

using CompressFunction = int (*)(const char *, size_t, char *, size_t);
using DecompressFunction = int (*)(const char *, size_t, char *, size_t);
using CompressIOVecFunction = int (*)(Buffer *, Buffer *);
using DempressIOVecFunction = int (*)(Buffer *, Buffer *);
using LeaseSizeFunction = int (*)(size_t);

class CompressHandler {
 public:
  CompressHandler() {
    // this->type = frpc::CompressTypeNone;
    this->compress = nullptr;
    this->decompress = nullptr;
    this->compress_iovec = nullptr;
    this->decompress_iovec = nullptr;
    this->lease_size = nullptr;
  }

  // int type;
  CompressFunction compress;
  DecompressFunction decompress;
  CompressIOVecFunction compress_iovec;
  DempressIOVecFunction decompress_iovec;
  LeaseSizeFunction lease_size;
};

class RPCCompressor {
 public:
  static RPCCompressor *get_instance() {
    static RPCCompressor kInstance;
    return &kInstance;
  }

 public:
  // parse message from compressed data
  // 		-1: error
  // 		-2, invalid compress type or decompress function
  int parse_from_compressed(const char *buf, size_t buflen, char *msg,
                            size_t msglen, int type) const;
  int parse_from_compressed(Buffer *src, Buffer *dest, int type) const;

  // serialized to compressed data
  // 		-1: error
  // 		-2, invalid compress type or compress function
  int serialize_to_compressed(const char *msg, size_t msglen, char *buf,
                              size_t buflen, int type) const;
  int serialize_to_compressed(Buffer *src, Buffer *dest, int type) const;

  /*
   * ret: >0: the theoretically lease size of compressed data
   * 		-1: error
   * 		-2, invalid compress type or lease_size function
   */
  int lease_compressed_size(int type, size_t origin_size) const;

  /*
   * Add existed compress_type
   * ret:  0, success
   * 		 1, handler existed and update success
   * 		-2, invalid compress type
   */
  int add(frpc::CompressType type);

  /*
   * Add self-define compress algorithm
   * ret:  0, success
   * 		 1, handler existed and update success
   * 		-2, invalid compress type or compress/decompress/lease_size
   * function
   */
  int add_handler(int type, CompressHandler &&handler);

  const CompressHandler *find_handler(int type) const;

  // clear all the registed handler
  void clear();

 private:
  RPCCompressor() {
    this->add(frpc::CompressTypeGzip);
    this->add(frpc::CompressTypeZlib);
    this->add(frpc::CompressTypeSnappy);
    this->add(frpc::CompressTypeLZ4);
  }

  CompressHandler handler[frpc::CompressTypeMax];
};

////////
// inl

inline int RPCCompressor::add_handler(int type, CompressHandler &&handler) {
  if (type >= frpc::CompressTypeMax || type <= frpc::CompressTypeNone)
    return -2;

  if (!handler.compress || !handler.decompress || !handler.lease_size)
    return -2;

  int ret = 0;

  if (this->handler[type].compress && this->handler[type].decompress &&
      this->handler[type].lease_size) {
    ret = 1;
  }

  this->handler[type] = std::move(handler);
  return ret;
}

inline const CompressHandler *RPCCompressor::find_handler(int type) const {
  if (type >= frpc::CompressTypeMax || type <= frpc::CompressTypeNone) {
    return NULL;
  }

  if (!this->handler[type].compress || !this->handler[type].decompress ||
      !this->handler[type].lease_size) {
    return NULL;
  }

  return &this->handler[type];
}

inline int RPCCompressor::parse_from_compressed(const char *buf, size_t buflen,
                                                char *msg, size_t msglen,
                                                int type) const {
  if (type >= frpc::CompressTypeMax || type <= frpc::CompressTypeNone ||
      !this->handler[type].decompress) {
    return -2;
  }

  return this->handler[type].decompress(buf, buflen, msg, msglen);
}

inline int RPCCompressor::parse_from_compressed(Buffer *src, Buffer *dest,
                                                int type) const {
  if (type >= frpc::CompressTypeMax || type <= frpc::CompressTypeNone ||
      !this->handler[type].decompress_iovec) {
    return -2;
  }

  return this->handler[type].decompress_iovec(src, dest);
}

inline int RPCCompressor::serialize_to_compressed(const char *msg,
                                                  size_t msglen, char *buf,
                                                  size_t buflen,
                                                  int type) const {
  if (type >= frpc::CompressTypeMax || type <= frpc::CompressTypeNone ||
      !this->handler[type].compress) {
    return -2;
  }

  return this->handler[type].compress(msg, msglen, buf, buflen);
}

inline int RPCCompressor::serialize_to_compressed(Buffer *src, Buffer *dest,
                                                  int type) const {
  if (type >= frpc::CompressTypeMax || type <= frpc::CompressTypeNone ||
      !this->handler[type].compress) {
    return -2;
  }

  return this->handler[type].compress_iovec(src, dest);
}

inline int RPCCompressor::lease_compressed_size(int type,
                                                size_t origin_size) const {
  if (type >= frpc::CompressTypeMax || type <= frpc::CompressTypeNone ||
      !this->handler[type].lease_size) {
    return -2;
  }

  return this->handler[type].lease_size(origin_size);
}

inline void RPCCompressor::clear() {
  for (int i = 0; i < frpc::CompressTypeMax; i++) {
    // this->handler[i].type = frpc::CompressTypeNone;
    this->handler[i].compress = nullptr;
    this->handler[i].decompress = nullptr;
    this->handler[i].lease_size = nullptr;
  }
}

}  // namespace MSF

#endif
