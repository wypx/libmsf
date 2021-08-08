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
#ifndef LIB_THREADLOCAL_H_
#define LIB_THREADLOCAL_H_

#include <pthread.h>

#include "Noncopyable.h"

namespace MSF {

template <typename T>
class ThreadLocal : noncopyable {
 public:
  ThreadLocal() { pthread_key_create(&pkey_, &ThreadLocal::destructor); }

  ~ThreadLocal() { pthread_key_delete(pkey_); }

  T& value() {
    T* perThreadValue = static_cast<T*>(pthread_getspecific(pkey_));
    if (!perThreadValue) {
      T* newObj = new T();
      pthread_setspecific(pkey_, newObj);
      perThreadValue = newObj;
    }
    return *perThreadValue;
  }

 private:
  static void destructor(void* x) {
    T* obj = static_cast<T*>(x);
    typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
    T_must_be_complete_type dummy;
    (void)dummy;
    delete obj;
  }

 private:
  pthread_key_t pkey_;
};

}  // namespace MSF