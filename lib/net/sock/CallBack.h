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
#ifndef SOCK_CALLBACK_H_
#define SOCK_CALLBACK_H_

#include <functional>

class Connector;

namespace MSF {

typedef std::shared_ptr<Connector> ConnectionPtr;
typedef std::function<void(const ConnectionPtr&)> ConnSuccCallback;
typedef std::function<void(const ConnectionPtr&)> CloseCallback;
typedef std::function<void(const ConnectionPtr&)> WriteCallback;
typedef std::function<void(const ConnectionPtr&)> ReadCallback;
typedef std::function<void(const ConnectionPtr&)> WriteCompleteCallback;
typedef std::function<void(const ConnectionPtr&, size_t)> HighWaterMarkCallback;

}  // namespace MSF
#endif
