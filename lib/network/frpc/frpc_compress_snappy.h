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
#ifndef FRPC_COMPRESS_SNAPPY_H_
#define FRPC_COMPRESS_SNAPPY_H_

#include <base/buffer.h>

namespace MSF {

class SnappyManager {
 public:
  /*
   * compress serialized msg into buf.
   * ret: -1: failed
   * 		>0: byte count of compressed data
   */
  static int SnappyCompress(const char *msg, size_t msglen, char *buf,
                            size_t buflen);

  /*
   * decompress and parse buf into msg
   * ret: -1: failed
   * 		>0: byte count of compressed data
   */
  static int SnappyDecompress(const char *buf, size_t buflen, char *msg,
                              size_t msglen);

  /*
   * compress Buffer src(Source) into Buffer dst(Sink)
   * ret: -1: failed
   * 		>0: byte count of compressed data
   */
  static int SnappyCompressIOVec(Buffer *src, Buffer *dst);

  /*
   * decompress Buffer src(Source) into Buffer dst(Sink)
   * ret: -1: failed
   * 		>0: byte count of compressed data
   */
  static int SnappyDecompressIOVec(Buffer *src, Buffer *dst);

  /*
   * lease size after compress origin_size data
   */
  static int SnappyLeaseSize(size_t origin_size);
};

}  // namespace MSF

#endif
