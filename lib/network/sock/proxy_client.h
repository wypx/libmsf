
// /**************************************************************************
//  *
//  * Copyright (c) 2017-2021, luotang.me <wypx520@gmail.com>, China.
//  * All rights reserved.
//  *
//  * Distributed under the terms of the GNU General Public License v2.
//  *
//  * This software is provided 'as is' with no explicit or implied warranties
//  * in respect of its properties, including, but not limited to, correctness
//  * and/or fitness for purpose.
//  *
//  **************************************************************************/

#include "proxy_connection.h"

namespace MSF {

class ProxyClient {
 public:
  ProxyClient() = default;
  ~ProxyClient() = default;

 private:
  ProxyConnection conn_;
  bool re_onnect_ = true; /* enable reconnct proxy server*/
  bool re_connecting_ = false;
};

typedef std::shared_ptr<ProxyClient> ProxyClientPtr;

}  // namespace MSF