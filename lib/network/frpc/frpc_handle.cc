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
#include "frpc_handle.h"

using namespace MSF;

namespace MSF {

FastRpcHandleFactory* FastRpcHandleFactory::Instance() {
  static FastRpcHandleFactory* g_instance;
  if (!g_instance) {
    static std::once_flag flag;
    std::call_once(flag, [&]() { g_instance = new FastRpcHandleFactory(); });
  }
  return g_instance;
}

template <typename... Args>
FastRpcHandlePtr FastRpcHandleFactory::CreateHandle(uint32_t type,
                                                    Args&&... args) {
  return FastRpcHandlePtr(
      Instance()->NewHandle(type, std::forward<Args>(args)...));
}

bool FastRpcHandleFactory::RegistHandle(const uint32_t type,
                                        const FastRpcHandleCreateCb& creator) {
  if (nullptr == creator) {
    return false;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  handle_creators_[type] = std::move(creator);
  return true;
}

template <typename... Args>
FastRpcHandle* FastRpcHandleFactory::NewHandle(const uint32_t type,
                                               Args&&... args) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = handle_creators_.find(type);
  if (it == handle_creators_.end()) {
    return nullptr;
  } else {
    FastRpcHandle* instance = it->second(std::forward<Args>(args)...);
    return instance;
  }
}

// template <typename... Args>
// void test(const uint32_t type, Args&&... args) {
//   FastRpcHandleFactory<FastRpcHandle, Args...>::Instance()->RegistHandle(
//       type, FastRpcHandleFactory<FastRpcHandle, Args...>::CreateObject);

//   FastRpcHandleFactory<FastRpcHandle, Args...>::Instance()->CreateHandle(
//       type, std::forward<Args>(args)...);

// }

// class FastRpcEchoRequest : public FastRpcHandle {
//  public:
//   FRPC_HANDLE_FACTORY_MEMBER(FastRpcEchoRequest);

//   void EntryInit(const frpc::Message& msg) {}
// };

// FRPC_HANDLE_FACTORY_IMPLEMENT(FastRpcEchoRequest, 1);
}  // namespace MSF