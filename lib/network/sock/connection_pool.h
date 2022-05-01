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
#ifndef SOCK_CONNECTION_POOL_H_
#define SOCK_CONNECTION_POOL_H_

#include "connection.h"
#include "event.h"

using namespace MSF;

namespace MSF {
class ConnnectionPool {
 public:
  ConnnectionPool() = default;
  ~ConnnectionPool() = default;
};

}  // namespace MSF
#endif
