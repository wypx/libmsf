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
#include "frpc_compress.h"

#include "frpc_compress_gzip.h"
#include "frpc_compress_lz4.h"
#include "frpc_compress_snappy.h"

#if 0
namespace MSF {

int RPCCompressor::add(frpc::CompressType type) {
  if (type >= SRPC_COMPRESS_TYPE_MAX || type <= RPCCompressNone) return -2;

  int ret = 0;

  if (this->handler[type].compress && this->handler[type].decompress &&
      this->handler[type].lease_size) {
    ret = 1;
  }

  // this->handler[type].type = type;

  switch (type) {
    case RPCCompressSnappy:
      this->handler[type].compress = SnappyManager::SnappyCompress;
      this->handler[type].decompress = SnappyManager::SnappyDecompress;
      this->handler[type].compress_iovec = SnappyManager::SnappyCompressIOVec;
      this->handler[type].decompress_iovec =
          SnappyManager::SnappyDecompressIOVec;
      this->handler[type].lease_size = SnappyManager::SnappyLeaseSize;
      break;
    case RPCCompressGzip:
      this->handler[type].compress = GzipCompress;
      this->handler[type].decompress = GzipDecompress;
      this->handler[type].compress_iovec = GzipCompressIOVec;
      this->handler[type].decompress_iovec = GzipDecompressIOVec;
      this->handler[type].lease_size = GzipLeaseSize;
      break;
    case RPCCompressZlib:
      this->handler[type].compress = ZlibCompress;
      this->handler[type].decompress = ZlibDecompress;
      this->handler[type].compress_iovec = ZlibCompressIOVec;
      this->handler[type].decompress_iovec = ZlibDecompressIOVec;
      this->handler[type].lease_size = ZlibLeaseSize;
      break;
    case RPCCompressLz4:
      this->handler[type].compress = LZ4Compress;
      this->handler[type].decompress = LZ4Decompress;
      this->handler[type].compress_iovec = LZ4CompressIOVec;
      this->handler[type].decompress_iovec = LZ4DecompressIOVec;
      this->handler[type].lease_size = LZ4LeaseSize;
      break;
    default:
      ret = -2;
      break;
  }

  return ret;
}

}  // namespace MSF

#endif
