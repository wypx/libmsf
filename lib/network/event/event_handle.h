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
#ifndef LIB_EVNETHANDLE_H_
#define LIB_EVNETHANDLE_H_

#include "Connector.h"
#include "Noncopyable.h"

using namespace MSF;

namespace MSF {

// https://blog.csdn.net/changqingcao/article/details/81036579
// https://blog.fatedier.com/2015/03/04/decoupling-by-using-reflect-and-simple-factory-pattern-in-cpp

class EventHandle : Noncopyable {
 public:
  EventHandle() {}
  explicit EventHandle(const Agent::AgentCommand cmd) { cmd = cmd_; }
  virtual ~EventHandle() {};
  virtual void handleMessage(ConnectionPtr c, void* head, void* body) {}
  virtual void handleMessage(ConnectionPtr c, Agent::AgentBhs& bhs) {}
  const Agent::AgentCommand cmd() const { return cmd_; }

 private:
  Agent::AgentCommand cmd_;
};

class EventHandleFactory : Noncopyable {
 public:
  typedef std::function<ClassHandle*(TArgs&&... args)> CreateClassFunc;

 public:
  EventHandleFactory() = default;
  ~EventHandleFactory() {};
  static EventHandleFactory& Instance() {
    static EventHandleFactory intance;
    return intance;
  }

  const uint32_t getHandleSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return registerFunctions_.size();
  }

  bool registHandle(const uint32_t cmd, const CreateClassFunc& func) {
    if (unlikely(nullptr == func)) {
      return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    // bool isReg =
    // registerFunctions_.insert(std::make_pair(std::string(typeName),
    // func)).second;
    registerFunctions_[cmd] = std::move(func);
    std::cout << "ClassHandleFactory Register cmd: " << cmd
              << " size:" << getHandleSize() << " this:" << this << std::endl;

    // auto it = registerFunctions_.begin();
    // while(it != registerFunctions_.end())
    // {
    //     std::cout<<"first->:"<< it->first << std::endl;
    //     // std::cout<<"second->:"<< std::hex << it->second << std::endl;
    //     it++;
    // }

    return true;
  }

  EventHandle* getHandle(const uint32_t cmd) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto itor = registerClass_.find(cmd);
    if (unlikely(itor == registerFunctions_.end())) {
      EventHandle* baseClass = itor->second();
      registerClass_[cmd] = baseClass;
    } else {
      return itor->second;
    }
  }

 private:
  mutable std::mutex mutex_;

  typedef std::function<EventHandle*()> EventHandleCreateFunc;
  std::unordered_map<uint32_t, EventHandleCreateFunc> registerFunctions_;
  std::map<uint32_t, EventHandle*> registerClass_;
};

template <typename T, typename... TArgs>
class EventHandleCreator {
 public:
  struct EventHandleRegister {
    EventHandleRegister() {
      char* demangleName = nullptr;
#ifdef __GNUC__
      demangleName =
          abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
#else
      // in this format?:     szDemangleName =  typeid(T).name();
      demangleName =
          abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
#endif
      std::string typeName;
      if (nullptr != demangleName) {
        typeName = demangleName;
        free(demangleName);
      }
      std::cout << "EventHandleRegister type: " << typeName << " cmd: " ++cmd
                << std::endl;
      EventHandleFactory<TArgs...>::Instance().Regist(cmd, CreateObject);
    }
  };

  static T* createObject(TArgs&&... args) {
    std::cout << abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr,
                                     nullptr) << std::endl;
    std::cout << "CreateObject now" << std::endl;
    T* t = nullptr;
    try {
      t = new T(std::forward<TArgs>(args)...);
    }
    catch (std::bad_alloc& e) {
      std::cout << "New object failed" << std::endl;
      return nullptr;
    }
    return t;
  }
  static EventHandleRegister register_;
};

class Login : public EventHandle, public EventHandleCreator<Login, int> {
 public:
  Login(const int cmd) { std::cout << "Create Cmd: " << cmd << std::endl; }
  virtual ~Login() {}

  virtual void handleMessage(const int fd, void* head, void* body) {
    std::cout << "CMD ClassHandle handle message " << std::endl;
  }

  virtual bool Init() { return true; }
};

void EventHandleTest() {
  EventHandle* p = EventHandleFactory<int>::Instance().Create(100, 100);
  p->handleMessage(0, nullptr, nullptr);

  EventHandleFactory::Instance().getHandle(100)->handleMessage(0, nullptr,
                                                               nullptr);
}

}  // namespace MSF
#endif