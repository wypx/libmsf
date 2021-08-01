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
#ifndef FRPC_BUFFER_H_
#define FRPC_BUFFER_H_

#include <google/protobuf/io/zero_copy_stream.h>

// https://www.dazhuanlan.com/gd1998/topics/1405434
// https://www.freesion.com/article/9426823971/
// https://blog.csdn.net/haowunanhai/article/details/79234446
// https://izualzhy.cn/protobuf-zerocopy
// https://blog.chiyl.info/index.php/archives/14/

namespace MSF {

class ReadBuffer : public google::protobuf::io::ZeroCopyInputStream {
 public:
  ReadBuffer();
  virtual ~ReadBuffer();

  // implements ZeroCopyInputStream ----------------------------------
  bool Next(const void** data, int* size) override;
  void BackUp(int count) override;
  bool Skip(int count) override;
  int64_t ByteCount() const override;
};

class WriteBuffer : public google::protobuf::io::ZeroCopyOutputStream {
 public:
  WriteBuffer();
  virtual ~WriteBuffer();

  // implements ZeroCopyOutputStream ---------------------------------
  bool Next(void** data, int* size);
  void BackUp(int count);
  int64_t ByteCount() const;
};
}
#endif