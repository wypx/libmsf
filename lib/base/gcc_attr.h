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

// Borrowed from
// https://code.google.com/p/gperftools/source/browse/src/base/thread_annotations.h
// but adapted for clang attributes instead of the gcc.
//
// This header file contains the macro definitions for thread safety
// annotations that allow the developers to document the locking policies
// of their multi-threaded code. The annotations can also help program
// analysis tools to identify potential thread safety issues.
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

// MSF_HAVE_ATTRIBUTE
//
// A function-like feature checking macro that is a wrapper around
// `__has_attribute`, which is defined by GCC 5+ and Clang and evaluates to a
// nonzero constant integer if the attribute is supported or 0 if not.
//
// It evaluates to zero if `__has_attribute` is not defined by the compiler.
//
// GCC: https://gcc.gnu.org/gcc-5/changes.html
// Clang: https://clang.llvm.org/docs/LanguageExtensions.html
#ifdef __has_attribute
#define MSF_HAVE_ATTRIBUTE(x) __has_attribute(x)
#else
#define MSF_HAVE_ATTRIBUTE(x) 0
#endif

#ifndef __attribute_noinline__
#if __has_attribute(noinline) || __GNUC_PREREQ(3, 1)
#define __attribute_noinline__ __attribute__((__noinline__))
#else
#define __attribute_noinline__
#endif
#endif

// MSF_ATTRIBUTE_SECTION
//
// Tells the compiler/linker to put a given function into a section and define
// `__start_ ## name` and `__stop_ ## name` symbols to bracket the section.
// This functionality is supported by GNU linker.  Any function annotated with
// `MSF_ATTRIBUTE_SECTION` must not be inlined, or it will be placed into
// whatever section its caller is placed into.
//
#ifndef MSF_ATTRIBUTE_SECTION
#define MSF_ATTRIBUTE_SECTION(name) \
  __attribute__((section(#name))) __attribute__((noinline))
#endif

// MSF_ATTRIBUTE_SECTION_VARIABLE
//
// Tells the compiler/linker to put a given variable into a section and define
// `__start_ ## name` and `__stop_ ## name` symbols to bracket the section.
// This functionality is supported by GNU linker.
#ifndef MSF_ATTRIBUTE_SECTION_VARIABLE
#define MSF_ATTRIBUTE_SECTION_VARIABLE(name) __attribute__((section(#name)))
#endif

// MSF_ATTRIBUTE_HOT, MSF_ATTRIBUTE_COLD
//
// Tells GCC that a function is hot or cold. GCC can use this information to
// improve static analysis, i.e. a conditional branch to a cold function
// is likely to be not-taken.
// This annotation is used for function declarations.
//
// Example:
//
//   int foo() __attribute_cold__;
#ifndef __attribute_cold__
#if __has_attribute(cold) || __GNUC_PREREQ(4, 3) || \
    (defined(__GNUC__) && !defined(__clang__))
#define __attribute_cold__ __attribute__((__cold__))
#else
#define __attribute_cold__
#endif
#endif

#ifndef __attribute_hot__
#if __has_attribute(hot) || __GNUC_PREREQ(4, 3) || \
    (defined(__GNUC__) && !defined(__clang__))
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

// Prevents the compiler from padding a structure to natural alignment
#if MSF_HAVE_ATTRIBUTE(packed) || (defined(__GNUC__) && !defined(__clang__))
#define __attribute_packed__ __attribute__((__packed__))
#else
#define __attribute_packed__
#endif

/* Force compiler to use inline*/
// Forces functions to either inline or not inline. Introduced in gcc 3.1.
#define __attribute_always_inline__ inline __attribute__((always_inline))

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

// Prevents the compiler from optimizing away stack frames for functions which
// end in a call to another function.
#if MSF_HAVE_ATTRIBUTE(disable_tail_calls)
#define MSF_HAVE_ATTRIBUTE_NO_TAIL_CALL 1
#define MSF_ATTRIBUTE_NO_TAIL_CALL __attribute__((disable_tail_calls))
#elif defined(__GNUC__) && !defined(__clang__)
#define MSF_HAVE_ATTRIBUTE_NO_TAIL_CALL 1
#define MSF_ATTRIBUTE_NO_TAIL_CALL \
  __attribute__((optimize("no-optimize-sibling-calls")))
#else
#define MSF_ATTRIBUTE_NO_TAIL_CALL
#define MSF_HAVE_ATTRIBUTE_NO_TAIL_CALL 0
#endif

// / MSF_ATTRIBUTE_NONNULL
//
// Tells the compiler either (a) that a particular function parameter
// should be a non-null pointer, or (b) that all pointer arguments should
// be non-null.
//
// Note: As the GCC manual states, "[s]ince non-static C++ methods
// have an implicit 'this' argument, the arguments of such methods
// should be counted from two, not one."
//
// Args are indexed starting at 1.
//
// For non-static class member functions, the implicit `this` argument
// is arg 1, and the first explicit argument is arg 2. For static class member
// functions, there is no implicit `this`, and the first explicit argument is
// arg 1.
//
// Example:
//
//   /* arg_a cannot be null, but arg_b can */
//   void Function(void* arg_a, void* arg_b) MSF_ATTRIBUTE_NONNULL(1);
//
//   class C {
//     /* arg_a cannot be null, but arg_b can */
//     void Method(void* arg_a, void* arg_b) MSF_ATTRIBUTE_NONNULL(2);
//
//     /* arg_a cannot be null, but arg_b can */
//     static void StaticMethod(void* arg_a, void* arg_b)
//     MSF_ATTRIBUTE_NONNULL(1);
//   };
//
// If no arguments are provided, then all pointer arguments should be non-null.
//
//  /* No pointer arguments may be null. */
//  void Function(void* arg_a, void* arg_b, int arg_c) MSF_ATTRIBUTE_NONNULL();
//
// NOTE: The GCC nonnull attribute actually accepts a list of arguments, but
// MSF_ATTRIBUTE_NONNULL does not.
#if MSF_HAVE_ATTRIBUTE(nonnull) || (defined(__GNUC__) && !defined(__clang__))
#define MSF_ATTRIBUTE_NONNULL(arg_index) __attribute__((nonnull(arg_index)))
#else
#define MSF_ATTRIBUTE_NONNULL(...)
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

// Tells the compiler that a given function never returns.
#if __GNUC__ > 2 || (defined(__GNUC__) && !defined(__clang__))
#define MSF_ATTRIBUTE_NORETURN __attribute__((noreturn))
#elif defined(_MSC_VER)
#define MSF_ATTRIBUTE_NORETURN __declspec(noreturn)
#else
#define MSF_ATTRIBUTE_NORETURN
#endif

#if defined(_MSC_VER)
// Note: Deprecation warnings seem to fail to trigger on Windows
// (https://bugs.chromium.org/p/webrtc/issues/detail?id=5368).
#define MSF_DEPRECATED __declspec(deprecated)
/* deprecated attribute support since gcc 3.1 */
#elif defined(__GNUC__) && \
    (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
#define MSF_DEPRECATED __attribute__((__deprecated__))
#else
#define MSF_DEPRECATED
#endif

// MSF_ATTRIBUTE_NO_SANITIZE_ADDRESS
//
// Tells the AddressSanitizer (or other memory testing tools) to ignore a given
// function. Useful for cases when a function reads random locations on stack,
// calls _exit from a cloned subprocess, deliberately accesses buffer
// out of bounds or does other scary things with memory.
// NOTE: GCC supports AddressSanitizer(asan) since 4.8.
// https://gcc.gnu.org/gcc-4.8/changes.html
#if defined(__GNUC__)
#define MSF_ATTRIBUTE_NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address))
#else
#define MSF_ATTRIBUTE_NO_SANITIZE_ADDRESS
#endif

// MSF_ATTRIBUTE_NO_SANITIZE_MEMORY
//
// Tells the  MemorySanitizer to relax the handling of a given function. All
// "Use of uninitialized value" warnings from such functions will be suppressed,
// and all values loaded from memory will be considered fully initialized.
// This attribute is similar to the ADDRESS_SANITIZER attribute above, but deals
// with initialized-ness rather than addressability issues.
// NOTE: MemorySanitizer(msan) is supported by Clang but not GCC.
#if defined(__clang__)
#define MSF_ATTRIBUTE_NO_SANITIZE_MEMORY __attribute__((no_sanitize_memory))
#else
#define MSF_ATTRIBUTE_NO_SANITIZE_MEMORY
#endif

// MSF_ATTRIBUTE_NO_SANITIZE_THREAD
//
// Tells the ThreadSanitizer to not instrument a given function.
// NOTE: GCC supports ThreadSanitizer(tsan) since 4.8.
// https://gcc.gnu.org/gcc-4.8/changes.html
#if defined(__GNUC__)
#define MSF_ATTRIBUTE_NO_SANITIZE_THREAD __attribute__((no_sanitize_thread))
#else
#define MSF_ATTRIBUTE_NO_SANITIZE_THREAD
#endif

// MSF_ATTRIBUTE_NO_SANITIZE_UNDEFINED
//
// Tells the UndefinedSanitizer to ignore a given function. Useful for cases
// where certain behavior (eg. division by zero) is being used intentionally.
// NOTE: GCC supports UndefinedBehaviorSanitizer(ubsan) since 4.9.
// https://gcc.gnu.org/gcc-4.9/changes.html
#if defined(__GNUC__) && \
    (defined(UNDEFINED_BEHAVIOR_SANITIZER) || defined(ADDRESS_SANITIZER))
#define MSF_ATTRIBUTE_NO_SANITIZE_UNDEFINED \
  __attribute__((no_sanitize("undefined")))
#else
#define MSF_ATTRIBUTE_NO_SANITIZE_UNDEFINED
#endif

// MSF_ATTRIBUTE_NO_SANITIZE_CFI
//
// Tells the ControlFlowIntegrity sanitizer to not instrument a given function.
// See https://clang.llvm.org/docs/ControlFlowIntegrity.html for details.
#if defined(__GNUC__) && defined(CONTROL_FLOW_INTEGRITY)
#define MSF_ATTRIBUTE_NO_SANITIZE_CFI __attribute__((no_sanitize("cfi")))
#else
#define MSF_ATTRIBUTE_NO_SANITIZE_CFI
#endif

// MSF_ATTRIBUTE_NO_SANITIZE_SAFESTACK
//
// Tells the SafeStack to not instrument a given function.
// See https://clang.llvm.org/docs/SafeStack.html for details.
#if defined(__GNUC__) && defined(SAFESTACK_SANITIZER)
#define MSF_ATTRIBUTE_NO_SANITIZE_SAFESTACK \
  __attribute__((no_sanitize("safe-stack")))
#else
#define MSF_ATTRIBUTE_NO_SANITIZE_SAFESTACK
#endif

// MSF_ATTRIBUTE_RETURNS_NONNULL
//
// Tells the compiler that a particular function never returns a null pointer.
#if MSF_HAVE_ATTRIBUTE(returns_nonnull) ||                       \
    (defined(__GNUC__) &&                                        \
     (__GNUC__ > 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)) && \
     !defined(__clang__))
#define MSF_ATTRIBUTE_RETURNS_NONNULL __attribute__((returns_nonnull))
#else
#define MSF_ATTRIBUTE_RETURNS_NONNULL
#endif

// MSF_MUST_USE_RESULT
//
// Tells the compiler to warn about unused results.
//
// When annotating a function, it must appear as the first part of the
// declaration or definition. The compiler will warn if the return value from
// such a function is unused:
//
//   MSF_MUST_USE_RESULT Sprocket* AllocateSprocket();
//   AllocateSprocket();  // Triggers a warning.
//
// When annotating a class, it is equivalent to annotating every function which
// returns an instance.
//
//   class MSF_MUST_USE_RESULT Sprocket {};
//   Sprocket();  // Triggers a warning.
//
//   Sprocket MakeSprocket();
//   MakeSprocket();  // Triggers a warning.
//
// Note that references and pointers are not instances:
//
//   Sprocket* SprocketPointer();
//   SprocketPointer();  // Does *not* trigger a warning.
//
// MSF_MUST_USE_RESULT allows using cast-to-void to suppress the unused result
// warning. For that, warn_unused_result is used only for clang but not for gcc.
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66425
//
// Note: past advice was to place the macro after the argument list.
#if MSF_HAVE_ATTRIBUTE(nodiscard)
#define MSF_MUST_USE_RESULT [[nodiscard]]
#elif defined(__clang__) && MSF_HAVE_ATTRIBUTE(warn_unused_result)
#define MSF_MUST_USE_RESULT __attribute__((warn_unused_result))
#else
#define MSF_MUST_USE_RESULT
#endif

// MSF_ATTRIBUTE_INITIAL_EXEC
//
// Tells the compiler to use "initial-exec" mode for a thread-local variable.
// See http://people.redhat.com/drepper/tls.pdf for the gory details.
#if MSF_HAVE_ATTRIBUTE(tls_model) || (defined(__GNUC__) && !defined(__clang__))
#define MSF_ATTRIBUTE_INITIAL_EXEC __attribute__((tls_model("initial-exec")))
#else
#define MSF_ATTRIBUTE_INITIAL_EXEC
#endif

// MSF_ATTRIBUTE_FUNC_ALIGN
//
// Tells the compiler to align the function start at least to certain
// alignment boundary
#if MSF_HAVE_ATTRIBUTE(aligned) || (defined(__GNUC__) && !defined(__clang__))
#define MSF_ATTRIBUTE_FUNC_ALIGN(bytes) __attribute__((aligned(bytes)))
#else
#define MSF_ATTRIBUTE_FUNC_ALIGN(bytes)
#endif

}  // namespace MSF
#endif
