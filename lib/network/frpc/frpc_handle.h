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
#ifndef FRPC_HANDLE_H_
#define FRPC_HANDLE_H_

#include <functional>
#include <mutex>
#include <unordered_map>

#include "frpc.pb.h"

namespace MSF {

class FastRpcController;
class FastRpcRequest;

typedef std::shared_ptr<FastRpcRequest> FastRpcRequestPtr;
typedef std::shared_ptr<FastRpcController> FastRpcControllerPtr;
typedef google::protobuf::Message GoogleMessage;
typedef std::shared_ptr<google::protobuf::Message> GoogleMessagePtr;

class FastRpcHandle : public std::enable_shared_from_this<FastRpcHandle> {
 public:
  FastRpcHandle() = default;
  virtual ~FastRpcHandle() = default;

  virtual void EntryInit(const GoogleMessage* msg) = 0;
};

typedef std::shared_ptr<FastRpcHandle> FastRpcHandlePtr;

// https://blog.csdn.net/changqingcao/article/details/81036579
// https://blog.fatedier.com/2015/03/04/decoupling-by-using-reflect-and-simple-factory-pattern-in-cpp
class FastRpcHandleFactory {
 public:
  typedef std::function<FastRpcHandle*()> FastRpcHandleCreateCb;

 public:
  static FastRpcHandleFactory* Instance();
  template <typename... Args>
  static FastRpcHandlePtr CreateHandle(uint32_t type, Args&&... args);

  FastRpcHandleFactory() = default;
  ~FastRpcHandleFactory() = default;
  bool RegistHandle(const uint32_t type, const FastRpcHandleCreateCb& creator);
  template <typename... Args>
  FastRpcHandle* NewHandle(const uint32_t type, Args&&... args);

 private:
  mutable std::mutex mutex_;
  std::unordered_map<uint32_t, FastRpcHandleCreateCb> handle_creators_;
};
}  // namespace MSF

#if 1
#define FRPC_HANDLE_FACTORY_IMPLEMENT(ClassName, type)                    \
  static ClassName* ClassName##CreateMySelf() { return new ClassName(); } \
  static bool __attribute__((unused)) FastRpcHandleFactory::Instance()    \
      ->RegistHandle(type, ClassName##CreateMySelf);

#define FRPC_HANDLE_FACTORY_MEMBER(ClassName)                        \
  ClassName();                                                       \
  virtual ~ClassName();                                              \
  virtual uint32_t HandleType();                                     \
  virtual const char* HandleName() { return #ClassName; }            \
  std::shared_ptr<ClassName> this_ptr() {                            \
    return std::dynamic_pointer_cast<ClassName>(shared_from_this()); \
  }
#endif

#endif