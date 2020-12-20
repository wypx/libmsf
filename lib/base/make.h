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
#ifndef BASE_MAKE_H_
#define BASE_MAKE_H_

#include <type_traits>  // std::decay
#include <utility>      // std::forward

namespace MSF {

// auto lc = MSF::make<std::unique_lock>(mutex_);

template <template <typename...> class Ret, typename... T>
using return_t = Ret<typename std::decay<T>::type...>;

template <template <typename...> class Ret, typename... T>
inline return_t<Ret, T...> make(T&&... args) {
  return return_t<Ret, T...>(std::forward<T>(args)...);
}
}