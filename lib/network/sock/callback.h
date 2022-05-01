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
#include <memory>

namespace MSF {
class Acceptor;
class Connector;
class Connection;

typedef std::shared_ptr<Acceptor> AcceptorPtr;
typedef std::function<void(const int, const uint16_t)> NewConnCallback;
typedef std::shared_ptr<Connection> ConnectionPtr;
typedef std::shared_ptr<Connector> ConnectorPtr;
typedef std::function<void(const ConnectionPtr&)> SuccCallback;
typedef std::function<void(const ConnectionPtr&)> CloseCallback;
typedef std::function<void(const ConnectionPtr&)> WriteCallback;
typedef std::function<void(const ConnectionPtr&)> ReadCallback;
typedef std::function<void(const ConnectionPtr&)> WriteIOCPCallback;
typedef std::function<void(const ConnectionPtr&, size_t)> HighWaterCallback;

}  // namespace MSF
#endif
