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
#ifndef BASE_ACTOR_H_
#define BASE_ACTOR_H_

#include <cxxabi.h>

#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <typeinfo>
#include <unordered_map>

#include "GccAttr.h"

using namespace MSF;

namespace MSF {

// https://blog.csdn.net/changqingcao/article/details/81036579
// https://blog.fatedier.com/2015/03/04/decoupling-by-using-reflect-and-simple-factory-pattern-in-cpp

class ClassHandle {
 public:
  ClassHandle() { std::cout << "ClassHandle construct" << std::endl; }
  virtual ~ClassHandle() {};
  virtual void handleMessage(const int fd, void* head, void* body) {
    std::cout << "ClassHandle handle message " << std::endl;
  }
};

void* ppppp;

template <typename... TArgs>
class ClassHandleFactory {
 public:
  typedef std::function<ClassHandle*(TArgs&&... args)> CreateClassFunc;

 public:
  // template<typename ...TArgs>
  // static ClassHandleFactory<TArgs...> & Instance() {
  //     // static ClassHandleFactory intance;
  //     static ClassHandleFactory<TArgs...> intance;
  //     return intance;
  // }
  static ClassHandleFactory* Instance() {
    // std::cout << "static ClassHandleFactory* Instance() 1" << " : " << ppppp
    // << std::endl;
    if (nullptr == ppppp) {
      // std::cout << "static ClassHandleFactory* Instance() 2" << std::endl;
      m_pActorFactory = new ClassHandleFactory();
      ppppp = m_pActorFactory;
    }
    return static_cast<ClassHandleFactory*>(ppppp);
  }
  virtual ~ClassHandleFactory() {};

  const uint32_t funcSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return registerFunctions_.size();
  }

  bool Regist(const uint32_t cmd, const CreateClassFunc& func) {
    if (unlikely(nullptr == func)) {
      return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    // bool isReg =
    // registerFunctions_.insert(std::make_pair(std::string(typeName),
    // func)).second;
    registerFunctions_[cmd] = std::move(func);
    std::cout << "ClassHandleFactory Register1 cmd: " << cmd
              << " size:" << funcSize() << " this:" << this << std::endl;

    auto it = registerFunctions_.begin();
    while (it != registerFunctions_.end()) {
      std::cout << "first->:" << it->first << std::endl;
      // std::cout<<"second->:"<< std::hex << it->second << std::endl;
      it++;
    }

    return true;
  }

  ClassHandle* Create(const uint32_t cmd, TArgs&&... args) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << "ClassHandleFactory Register2 cmd: " << cmd
              << " size:" << funcSize() << " this:" << this << std::endl;

    auto it = registerFunctions_.begin();
    while (it != registerFunctions_.end()) {
      std::cout << "first->:" << it->first << std::endl;
      // std::cout<<"second->:"<< std::hex << it->second << std::endl;
      it++;
    }

    auto itor = registerFunctions_.find(cmd);
    if (unlikely(itor == registerFunctions_.end())) {
      std::cout << "ClassHandle Regist func not found " << std::endl;
      return nullptr;
    } else {
      std::cout << "ClassHandle ccc0 " << std::endl;
      ClassHandle* baseClass = itor->second(std::forward<TArgs>(args)...);
      std::cout << "ClassHandle ccc1 " << std::endl;
      registerClass_[cmd] = baseClass;
      std::cout << "ClassHandle ccc2 " << std::endl;
      return baseClass;
    }
  }

  ClassHandle* GetClassHandle(const uint32_t cmd) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto itor = registerClass_.find(cmd);
    if (unlikely(itor == registerFunctions_.end())) {
      return nullptr;
    } else {
      return itor->second;
    }
  }

 private:
  ClassHandleFactory() {}
  mutable std::mutex mutex_;
  std::unordered_map<uint32_t, std::function<ClassHandle*(TArgs&&...)>>
      registerFunctions_;
  std::map<uint32_t, ClassHandle*> registerClass_;

  static ClassHandleFactory<TArgs...>* m_pActorFactory;
};

template <typename... TArgs>
ClassHandleFactory<TArgs...>* ClassHandleFactory<TArgs...>::m_pActorFactory;

enum TestCmd {
  TEST_CMD1 = 10,
  TEST_CMD2 = 11
};

static uint32_t cmd = 99;

template <typename T, typename... TArgs>
class DynamicCreator {
 public:
  struct Register {
    Register() {
      std::cout << "DynamicCreator Register construct: " << this << std::endl;
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
      std::cout << "DynamicCreator Register typeName1: " << typeName
                << std::endl;

      std::cout << "DynamicCreator Register cmd: " << ++cmd << std::endl;
      ClassHandleFactory<TArgs...>::Instance()->Regist(cmd, CreateObject);
      // std::cout << "DynamicCreator Register typeName2: " << typeName <<
      // std::endl;
    }
    inline void do_nothing() const {};
  };

  DynamicCreator() {
    std::cout << "DynamicCreator construct" << std::endl;
    register_.do_nothing();
  }

  virtual ~DynamicCreator() {
    register_.do_nothing();
  };

  static T* CreateObject(TArgs&&... args) {
    std::cout << abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr,
                                     nullptr) << std::endl;
    std::cout << "static Actor* DynamicCreator::CreateObject(Targs... args)"
              << std::endl;
    T* t = nullptr;
    try {
      t = new T(std::forward<TArgs>(args)...);
    }
    catch (std::bad_alloc& e) {
      std::cout << "new object" << std::endl;
      return nullptr;
    }
    return t;
  }
  static Register register_;
};

template <typename T, typename... TArgs>
typename DynamicCreator<T, TArgs...>::Register
    DynamicCreator<T, TArgs...>::register_;

class Cmd : public ClassHandle, public DynamicCreator<Cmd, std::string, int> {
 public:
  Cmd(const std::string& s, const int cmd) {
    std::cout << "Create Cmd: " << cmd << std::endl;
    // std::cout << "Create s: " << s << std::endl;
  }

  virtual void handleMessage(const int fd, void* head, void* body) {
    std::cout << "CMD ClassHandle handle message " << std::endl;
  }
  virtual ~Cmd() {}

  virtual bool Init() { return true; }
};

class Step : public ClassHandle,
             public DynamicCreator<Step, std::string&, int> {
 public:
  Step(const std::string& strType, const int iSeq) {
    std::cout << "Create Cmd: " << iSeq << std::endl;
    // std::cout << "Create " << strType << " with seq " << iSeq << std::endl;
  }
  virtual void handleMessage(const int fd, void* head, void* body) {
    std::cout << "Step ClassHandle handle message " << std::endl;
  }
  virtual ~Step() {}
  virtual bool Init() { return true; }
};

class Worker {
 public:
  template <typename... TArgs>
  ClassHandle* CreateActor(const uint32_t cmd, TArgs&&... args) {
    ClassHandle* p = ClassHandleFactory<TArgs...>::Instance()->Create(
        cmd, std::forward<TArgs>(args)...);
    if (p == nullptr) {
      std::cout << "Create actor failed" << std::endl;
    }
    std::cout << "Create actor suss" << std::endl;
    return p;
  }
  // template<typename ...TArgs>
  // Cmd* MakeSharedCmd(const uint32_t c, TArgs&&... args)
  // {
  //     Cmd* cmd =
  //     dynamic_cast<Cmd*>(ClassHandleFactory<TArgs...>::Instance()->Create(c,std::forward<TArgs>(args)...));
  //     if (nullptr == cmd)
  //     {
  //       // LOG4_ERROR("failed to make shared cmd \"%s\"",
  //       strCmdName.c_str()); return nullptr;
  //     }
  //     return cmd;
  // }
  // MakeSharedCmd(oCmdConf["cmd"][i]("class"), iCmd);
};

void ActorTest() {
  // Actor* p1 = ActorFactory<std::string,
  // int>::Instance()->Create(std::string("Cmd"), std::string("neb::Cmd"),
  // 1001);
  // Actor* p3 = ActorFactory<>::Instance()->Create(std::string("Cmd"));
  Worker W;
  std::cout << "---------------------------------------------------------------"
               "-------" << std::endl;
  ClassHandle* p1 = W.CreateActor(100, "CLASS CMD", 1000);
  p1->handleMessage(0, nullptr, nullptr);
  std::cout << abi::__cxa_demangle(typeid(Worker).name(), nullptr, nullptr,
                                   nullptr) << std::endl;
  std::cout << "---------------------------------------------------------------"
               "-------" << std::endl;
  ClassHandle* p2 = W.CreateActor(101, "CLASS STEP", 10001);
  std::cout << "---------------------------------------------------------------"
               "-------" << std::endl;
  p2->handleMessage(0, nullptr, nullptr);
}

}  // namespace MSF
#endif