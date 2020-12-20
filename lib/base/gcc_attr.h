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
#ifndef BASE_GCCATTR_H_
#define BASE_GCCATTR_H_

#include <sys/cdefs.h>

namespace MSF {

//其他属性:
// https://www.jianshu.com/p/e2dfccc32c80
// https://www.cnblogs.com/songbingyu/p/3894096.html

/* 两次转换将宏的值转成字符串 */
#define _MSF_MACROSTR(a) (#a)
#define MSF_MACROSTR(a) (_MSF_MACROSTR(a))

/* If supported, give compiler hints for branch prediction. */
#if !defined(__GNUC__) || (__GNUC__ == 2 && __GNUC_MINOR__ < 96)
#define __builtin_expect(x, expected_value) (x)
#endif

/* linux-2.6.38.8/include/linux/compiler.h */
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

/* linux-2.6.38.8/include/linux/compiler.h */
#ifndef likely
#define likely(x) (__builtin_constant_p(x) ? !!(x) : __branch_check__(x, 1))
#endif
#ifndef unlikely
#define unlikely(x) (__builtin_constant_p(x) ? !!(x) : __branch_check__(x, 0))
#endif

#ifndef __has_attribute /* clang */
#define __has_attribute(x) 0
#endif

#ifndef __attribute_noinline__
#if __has_attribute(noinline) || __GNUC_PREREQ(3, 1)
#define __attribute_noinline__ __attribute__((__noinline__))
#else
#define __attribute_noinline__
#endif
#endif

#ifndef __attribute_cold__
#if __has_attribute(cold) || __GNUC_PREREQ(4, 3)
#define __attribute_cold__ __attribute__((__cold__))
#else
#define __attribute_cold__
#endif
#endif

#ifndef __attribute_hot__
#if __has_attribute(hot) || __GNUC_PREREQ(4, 3)
#define __attribute_hot__ __attribute__((__hot__))
#else
#define __attribute_hot__
#endif
#endif
#ifndef __attribute_noreturn__
#if __has_attribute(noreturn) || __GNUC_PREREQ(2, 5)
#define __attribute_noreturn__ __attribute__((__noreturn__))
#else
#define __attribute_noreturn__
#endif
#endif

#ifndef __attribute_fallthrough__
#if __has_attribute(fallthrough) || __GNUC_PREREQ(7, 0)
#define __attribute_fallthrough__ __attribute__((__fallthrough__));
#else
#define __attribute_fallthrough__ /* fall through */
#endif
#endif

#ifndef __attribute_format__
#if __has_attribute(format) || __GNUC_PREREQ(2, 95) /*(maybe earlier gcc, \
                                                       too)*/
#define __attribute_format__(x) __attribute__((__format__ x))
#else
#define __attribute_format__(x)
#endif
#endif

#ifndef __attribute_pure__
#if __has_attribute(pure) || __GNUC_PREREQ(2, 96)
#define __attribute_pure__ __attribute__((__pure__))
#else
#define __attribute_pure__
#endif
#endif
// #define __attribute_const__  __attribute__((const))
// #define __attribute_const__  __attribute__((const))

/* 这个表示一个方法的返回值只由参数决定, 如果参数不变的话,
 *  就不再调用此函数，直接返回值.经过我的尝试发现还是调用了,
 *  后又经查资料发现要给gcc加一个-O的参数才可以.
 *  是对函数调用的一种优化*/
#define MSF_CONST_CALL __attribute__((const))

/* 表示函数的返回值必须被检查或使用,否则会警告*/

#if defined(_MSC_VER)
#define __attribute_unused__ __pragma(warning(suppress : 4100 4101 4189))
#elif defined(__GNUC__)
#define __attribute_unused__ __attribute__((unused))
// #define __attribute_unused__ __attribute__((__unused__))
#else
#define __attribute_unused__
#endif

#define __attribute_packed__ __attribute__((__packed__))

// #define __attribute_weak__ __attribute__((weak))
// #define __attribute_noreturn__ __attribute__((__noreturn__))
// #define __attribute_always_inline__ inline __attribute__((always_inline))

/* Force compiler to use inline*/
#define MSF_ALWAYS_INLINE inline __attribute__((always_inline))
#define MSF_LIBRARY_INITIALIZER(func, level)                  \
  static void func(void) __attribute__((constructor(level))); \
  static void func(void)

// static void __attribute__((constructor, used))

// https://www.it1352.com/463497.html
// https://www.icode9.com/content-4-501004.html
//  __attribute__ ((init_priority (2000)))

#define MSF_LIBRARY_FINALIZER(func)                   \
  static void func(void) __attribute__((destructor)); \
  static void func(void)

/** when static library not been linked,
 *  this check is needed. */
#ifndef MSF_ATTRIBUTE_WIKE
#define MSF_ATTRIBUTE_WIKE __attribute__((weak))
#else
#define MSF_ATTRIBUTE_WIKE
#endif

#ifndef MSF_ATTRIBUTE_WIKE_ALIAS
#define MSF_ATTRIBUTE_WIKE_ALIAS(func) \
  __attribute__((weak, alias(MSF_MACROSTR(func))))
#else
#define MSF_ATTRIBUTE_WIKE_ALIAS
#endif

#ifndef MSF_ATTRIBUTE_WIKEREF_ALIAS
#define MSF_ATTRIBUTE_WIKEREF_ALIAS(func) \
  __attribute__((weakref, alias(MSF_MACROSTR(func))))
#else
#define MSF_ATTRIBUTE_WIKEREF_ALIAS
#endif

/* 函数别名 3种方式 see SocketTcpNagleOff */
// https://bbs.csdn.net/topics/390522766
// https://cloud.tencent.com/developer/ask/42322/answer/64049
// https://stackoverflow.com/questions/9864125/c11-how-to-alias-a-function
#define ALIAS_FUNCTION(OriginalnamE, AliasnamE)               \
  template <typename... Args>                                 \
  inline auto AliasnamE(Args&&... args)                       \
      ->decltype(OriginalnamE(std::forward<Args>(args)...)) { \
    return OriginalnamE(std::forward<Args>(args)...);         \
  }

#if __GNUC__ > 2
#define MSF_NORETURN __attribute__((__noreturn__))
#else
#define MSF_NORETURN
#endif

/* deprecated attribute support since gcc 3.1 */
#if defined __GNUC__&&(__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
#define MSF_DEPRECATED __attribute__((__deprecated__))
#else
#define MSF_DEPRECATED
#endif

}  // namespace MSF
#endif
