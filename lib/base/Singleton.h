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
#ifndef BASE_SINGLETON_H_
#define BASE_SINGLETON_H_

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>  // atexit

#include "Noncopyable.h"

namespace MSF {

// https://blog.csdn.net/sai_j/article/details/82766225

// This doesn't detect inherited member functions!
// http://stackoverflow.com/questions/1966362/sfinae-to-check-for-inherited-member-functions
template <typename T>
struct has_no_destroy {
  template <typename C>
  static char test(decltype(&C::no_destroy));
  template <typename C>
  static int32_t test(...);
  const static bool value = sizeof(test<T>(0)) == 1;
};
}  // namespace MSF

template <typename T>
class Singleton : Noncopyable {
 public:
  Singleton() = delete;
  ~Singleton() = delete;

  // 1.
  static T& instance() {
    pthread_once(&ponce_, &Singleton::init);
    assert(value_ != NULL);
    return *value_;
  }

 private:
  static std::unique_ptr<Singleton> g_Singleton;

 public:
  // https://blog.csdn.net/xijiacun/article/details/71023777
  // https://www.tuicool.com/articles/QbmInyF
  // https://blog.csdn.net/u011726005/article/details/82356538
  /** Using singleton Singleton instance in one process */
  // pthread_once
  // https://blog.csdn.net/sjin_1314/article/details/10934239
  // 2.
  static Singleton& GetSingleton() {
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [&]() { g_Singleton.reset(new Singleton()); });
    return *g_Singleton;
  }
  // 3.
  static Singleton& GetInstance() {
    static Singleton intance;
    return intance;
  }

 private:
  static void init() {
    value_ = new T();
    if (!has_no_destroy<T>::value) {
      ::atexit(destroy);
    }
  }

  static void destroy() {
    typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
    T_must_be_complete_type dummy;
    (void)dummy;

    delete value_;
    value_ = NULL;
  }

 private:
  static pthread_once_t ponce_;
  static T* value_;
};

template <typename T>
pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

template <typename T>
T* Singleton<T>::value_ = NULL;
}
#endif