/**************************************************************************
 *
 * Copyright (c) 2017-2018, luotang.me <wypx520@gmail.com>, China.
 * All rights reserved.
 *
 * Distributed under the terms of the GNU General Public License v2.
 *
 * This software is provided 'as is' with no explicit or implied warranties
 * in respect of its properties, including, but not limited to, correctness
 * and/or fitness for purpose.
 *
 **************************************************************************/
#ifndef __MSF_UTILS_H__
#define __MSF_UTILS_H__

#include <base/Define.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <cctype> /* uppper()*/
#include <cerrno>
#include <climits>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

using namespace MSF::BASE;

namespace MSF {
namespace BASE {

// https://blog.csdn.net/ccmmfit/article/details/45440537
// https://www.cnblogs.com/didiaodidiao/p/9398361.html
/* Notice: var pointer is need */
#define MSF_SWAP(a, b) \
  {                    \
    *a = *a ^ *b;      \
    *b = *a ^ *b;      \
    *a = *a ^ *b;

#ifndef MSF_ARRAY_SIZE
#define MSF_ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

#define MSF_UNUSED(expr) \
  do {                   \
    (void)(expr);        \
  } while (0)

/** Replacement for offsetof on platforms that don't define it. */
#ifndef offsetof
#define offsetof(type, field) ((off_t)(&((type *)0)->field))
#else
// #define offsetof(type, field)      offsetof((type, filed)
#endif

#define container_of(ptr, type, member)                \
  ({                                                   \
    const typeof(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); \
  })

/*Align by number of align*/
#define MSF_ALGIN_UP(num, align)    (((num)+(align-1) / (align))
#define MSF_ROUNDUP(num, align) ((((num) + (align - 1)) / (align)) * (align))
#define MSF_ABS(value) (((value) >= 0) ? (value) : -(value))

/*Align by number of 2 times*/
#define MSF_ALIGN(d, a) (((d) + (a - 1)) & ~(a - 1))
#define MSF_ALIGN_PTR(p, a) \
  (uint8_t)(((uint32_t)(p) + ((uint32_t)a - 1)) & ~((uint32_t)a - 1))

/* Avoid warnings on solaris, where isspace()
 * is an index into an array, and gcc uses signed chars */
#define MSF_ISSPACE(c) isspace((MSF_U8)c)

#define MSF_TOLOWER(c) (__uint8)((c >= 'A' && c <= 'Z') ? (c | 0x20) : c)
#define MSF_TOUPPER(c) (__uint8)((c >= 'a' && c <= 'z') ? (c & ~0x20) : c)
#define MSF_STRNCMP(s1, s2, n) strncmp((const char *)s1, (const char *)s2, n)
#define MSF_SNPRINTF snprintf

/* msvc and icc7 compile strcmp() to inline loop */
#define MSF_STRCMP(s1, s2) strcmp((const char *)s1, (const char *)s2)
#define MSF_STRSTR(s1, s2) strstr((const char *)s1, (const char *)s2)
#define MSF_STRLEN(s) strlen((const char *)s)
#define MSF_STRCHR(s1, c) strchr((const char *)s1, (char)c)

#ifndef va_copy
#ifdef __va_copy
#define MSF_VA_COPY __va_copy
#else
#define MSF_VA_COPY(x, y) (x) = (y)
#endif
#endif

#include <stdint.h>

void msf_nsleep(int ns);
void msf_susleep(int s, int us);
bool msf_safe_strtoull(const char *str, uint64_t *out);
bool msf_safe_strtoll(const char *str, uint64_t *out);
bool msf_safe_strtoul(const char *str, uint32_t *out);
bool msf_safe_strtol(const char *str, int32_t *out);

extern size_t strlcpy(char *dst, const char *src, size_t size);
extern size_t strlcat(char *dst, const char *src, size_t size);

// Use implicit_cast as a safe version of static_cast or const_cast
// for upcasting in the type hierarchy (i.e. casting a pointer to Foo
// to a pointer to SuperclassOfFoo or casting a pointer to Foo to
// a const pointer to Foo).
// When you use implicit_cast, the compiler checks that the cast is safe.
// Such explicit implicit_casts are necessary in surprisingly many
// situations where C++ demands an exact type match instead of an
// argument type convertable to a target type.
//
// The From type can be inferred, so the preferred syntax for using
// implicit_cast is the same as for static_cast etc.:
//
//   implicit_cast<ToType>(expr)
//
// implicit_cast would have been part of the C++ standard library,
// but the proposal was submitted too late.  It will probably make
// its way into the language in the future.
template <typename To, typename From>
inline To implicit_cast(From const &f) {
  return f;
}
// When you upcast (that is, cast a pointer from type Foo to type
// SuperclassOfFoo), it's fine to use implicit_cast<>, since upcasts
// always succeed.  When you downcast (that is, cast a pointer from
// type Foo to type SubclassOfFoo), static_cast<> isn't safe, because
// how do you know the pointer is really of type SubclassOfFoo?  It
// could be a bare Foo, or of type DifferentSubclassOfFoo.  Thus,
// when you downcast, you should use this macro.  In debug mode, we
// use dynamic_cast<> to double-check the downcast is legal (we die
// if it's not).  In normal mode, we do the efficient static_cast<>
// instead.  Thus, it's important to test in debug mode to make sure
// the cast is legal!
//    This is the only place in the code we should use dynamic_cast<>.
// In particular, you SHOULDN'T be using dynamic_cast<> in order to
// do RTTI (eg code like this:
//    if (dynamic_cast<Subclass1>(foo)) HandleASubclass1Object(foo);
//    if (dynamic_cast<Subclass2>(foo)) HandleASubclass2Object(foo);
// You should design the code some other way not to need this.

template <typename To, typename From>  // use like this: down_cast<T*>(foo);
inline To down_cast(From *f)           // so we only accept pointers
{
  // Ensures that To is a sub-type of From *.  This test is here only
  // for compile-time type checking, and has no overhead in an
  // optimized build at run-time, as it will be optimized away
  // completely.
  if (false) {
    implicit_cast<From *, To>(0);
  }

#if !defined(NDEBUG) && !defined(GOOGLE_PROTOBUF_NO_RTTI)
  assert(f == NULL || dynamic_cast<To>(f) != NULL);  // RTTI: debug mode only!
#endif
  return static_cast<To>(f);
}

// struct ExitCaller: private Noncopyable {
//     ~ExitCaller() { functor_(); }
//     ExitCaller(std::function<void()>&& functor): functor_(std::move(functor))
//     {}
// private:
//     std::function<void()> functor_;
// };

}  // namespace BASE
}  // namespace MSF

inline int get_value() {
  // read_lock
  //_bittset test_bit
  // atuomoc_read
  // automic_des/inc
  // rwlock_t
  // mm-segment_t fs = get_fs() set_fs_get_ds())
  // kmalloc vmalooc
  // semphone

  // ipref带宽丢包抖动 netpref qpref时延
  // vnsatat
  // sar监控
  // mleak内存监控
  // 0拷贝 节省用户态到内核态的拷贝
  //分布式存储解决方案
  // system/popen命令注入风险 execve代替
  //有= %#8x
  // sparseg工具
  //_rcu
  // hping

  return 0;
}

#endif
