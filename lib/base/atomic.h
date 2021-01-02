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
#ifndef BASE_ATOMIC_H_
#define BASE_ATOMIC_H_

#include <stdint.h>

#include "utils.h"

#if defined(WIN32) || defined(WIN64)
// clang-format off
// clang formating would change include order.

// Include winsock2.h before including <windows.h> to maintain consistency with
// win32.h. To include win32.h directly, it must be broken out into its own
// build target.
#include <winsock2.h>
#include <windows.h>
// clang-format on
#endif

namespace MSF {

#ifndef HAVE_GCC_ATOMIC
#define HAVE_GCC_ATOMIC 1
#endif

class AtomicOps {
 public:
  /* GCC 4.1 builtin atomic operations */
  typedef volatile uint32_t atomic_t;

#if defined(WIN32) || defined(WIN64) || defined(__CYGWIN__)
  // Assumes sizeof(int) == sizeof(LONG), which it is on Win32 and Win64.
  static int Increment(volatile int* i) {
    return ::InterlockedIncrement(reinterpret_cast<volatile LONG*>(i));
  }
  static int Decrement(volatile int* i) {
    return ::InterlockedDecrement(reinterpret_cast<volatile LONG*>(i));
  }
  static int AcquireLoad(volatile const int* i) { return *i; }
  static void ReleaseStore(volatile int* i, int value) { *i = value; }
  static int CompareAndSwap(volatile int* i, int old_value, int new_value) {
    return ::InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(i),
                                        new_value, old_value);
  }
  // Pointer variants.
  template <typename T>
  static T* AcquireLoadPtr(T* volatile* ptr) {
    return *ptr;
  }
  template <typename T>
  static T* CompareAndSwapPtr(T* volatile* ptr, T* old_value, T* new_value) {
    return static_cast<T*>(::InterlockedCompareExchangePointer(
        reinterpret_cast<PVOID volatile*>(ptr), new_value, old_value));
  }

#elif defined(HAVE_GCC_ATOMIC)
#define AtomicCmpSet(lock, old, set) \
  __sync_bool_compare_and_swap(lock, old, set)
#define AtomicAdd(value, add) __sync_add_and_fetch(value, add)
#define AtomicFetchAdd(value, add) __sync_fetch_and_add(value, add)
#define AtomicFetchSub(value, sub) __sync_fetch_and_sub(value, sub)
#define AtomicSub(value, sub) __sync_sub_and_fetch(value, sub)
#define AtomicGet(value, dstvar)             \
  do {                                       \
    dstvar = __sync_sub_and_fetch(value, 0); \
  } while (0)

#define MemoryBarrier() __sync_synchronize()

#if (__i386__ || __i386 || __amd64__ || __amd64)
#define CpuPause() __asm__("pause")
#else
#define CpuPause()
#endif
#elif(__amd64__ || __amd64)
#define kSmpLock "lock;"
#define MemoryBarrier() __asm__ volatile("" :: : "memory")
#define CpuPause() __asm__("pause")
  //#define CpuPause() _mm_pause();

  /*
   * "cmpxchgq  r, [m]":
   *
   *     if (rax == [m]) {
   *         zf = 1;
   *         [m] = r;
   *     } else {
   *         zf = 0;
   *         rax = [m];
   *     }
   *
   *
   * The "r" is any register, %rax (%r0) - %r16.
   * The "=a" and "a" are the %rax register.
   * Although we can return result in any register, we use "a" because it is
   * used in cmpxchgq anyway.  The result is actually in %al but not in $rax,
   * however as the code is inlined gcc can test %al as well as %rax.
   *
   * The "cc" means that flags were changed.
   */
  inline atomic_t AtomicCmpSet(atomic_t* lock, atomic_t old, atomic_t set) {
    uint8_t res;
    __asm__ volatile(kSmpLock
                     "    cmpxchgq  %3, %1;   "
                     "    sete      %0;       "
                     : "=a"(res)
                     : "m"(*lock), "a"(old), "r"(set)
                     : "cc", "memory");
    return res;
  }

  /*
   * "xaddq  r, [m]":
   *
   *     temp = [m];
   *     [m] += r;
   *     r = temp;
   *
   *
   * The "+r" is any register, %rax (%r0) - %r16.
   * The "cc" means that flags were changed.
   */
  inline atomic_t AtomicFetchAdd(atomic_t* value, atomic_t add) {
    __asm__ volatile(kSmpLock "    xaddq  %0, %1;   "
                     : "+r"(add)
                     : "m"(*value)
                     : "cc", "memory");
    return add;
  }

#elif(__i386__ || __i386) /* x86*/

  /*
  * "cmpxchgl  r, [m]":
  *
  *     if (eax == [m]) {
  *         zf = 1;
  *         [m] = r;
  *     } else {
  *         zf = 0;
  *         eax = [m];
  *     }
  *
  *
  * The "r" means the general register.
  * The "=a" and "a" are the %eax register.
  * Although we can return result in any register, we use "a" because it is
  * used in cmpxchgl anyway.  The result is actually in %al but not in %eax,
  * however, as the code is inlined gcc can test %al as well as %eax,
  * and icc adds "movzbl %al, %eax" by itself.
  *
  * The "cc" means that flags were changed.
  */

  inline atomic_t AtomicCmpSet(atomic_t* lock, atomic_t old, atomic_t set) {
    uint8_t res;
    __asm__ volatile(kSmpLock
                     "    cmpxchgl  %3, %1;   "
                     "    sete      %0;       "
                     : "=a"(res)
                     : "m"(*lock), "a"(old), "r"(set)
                     : "cc", "memory");
    return res;
  }

/*
* "xaddl  r, [m]":
*
*     temp = [m];
*     [m] += r;
*     r = temp;
*
*
* The "+r" means the general register.
* The "cc" means that flags were changed.
*/

#if !((__GNUC__ == 2 && __GNUC_MINOR__ <= 7) || (__INTEL_COMPILER >= 800))

  /*
  * icc 8.1 and 9.0 compile broken code with -march=pentium4 option:
  * ngx_atomic_fetch_add() always return the input "add" value,
  * so we use the gcc 2.7 version.
  *
  * icc 8.1 and 9.0 with -march=pentiumpro option or icc 7.1 compile
  * correct code.
  */

  inline atomic_t AtomicFetchAdd(atomic_t* value, atomic_t add) {
    __asm__ volatile(kSmpLock "    xaddl  %0, %1;   "
                     : "+r"(add)
                     : "m"(*value)
                     : "cc", "memory");

    return add;
  }

#else

  /*
  * gcc 2.7 does not support "+r", so we have to use the fixed
  * %eax ("=a" and "a") and this adds two superfluous instructions in the end
  * of code, something like this: "mov %eax, %edx / mov %edx, %eax".
  */

  inline atomic_t AtomicFetchAdd(atomic_t* value, atomic_t add) {
    atomic_t old;

    __asm__ volatile(kSmpLock "    xaddl  %2, %1;   "
                     : "=a"(old)
                     : "m"(*value), "a"(add)
                     : "cc", "memory");

    return old;
  }

#endif

/*
* on x86 the write operations go in a program order, so we need only
* to disable the gcc reorder optimizations
*/

#define MemoryBarrier() __asm__ volatile("" :: : "memory")
/* old "as" does not support "pause" opcode */
#define CpuPause() __asm__(".byte 0xf3, 0x90")

#else

/* Implementation using __atomic macros. */
#define AtomicFetchAdd(var, count) \
  __atomic_add_fetch(&var, (count), __ATOMIC_RELAXED)
#define AtomicFetchSub(var, count) \
  __atomic_sub_fetch(&var, (count), __ATOMIC_RELAXED)
#define AtomicAdd(var, oldvalue_var, count)                             \
  do {                                                                  \
    oldvalue_var = __atomic_fetch_add(&var, (count), __ATOMIC_RELAXED); \
  } while (0)

#define AtomicGet(var, dstvar)                        \
  do {                                                \
    dstvar = __atomic_load_n(&var, __ATOMIC_RELAXED); \
  } while (0)

  static int AtomicAcquireLoad(volatile const int* i) {
    return __atomic_load_n(i, __ATOMIC_ACQUIRE);
  }

  static void AtomicReleaseStore(volatile int* i, int value) {
    __atomic_store_n(i, value, __ATOMIC_RELEASE);
  }
#define AtomicSet(var, value) __atomic_store_n(&var, value, __ATOMIC_RELAXED)

  static int AtomicCompareAndSwap(volatile int* i, int old_value,
                                  int new_value) {
    return __sync_val_compare_and_swap(i, old_value, new_value);
  }
  // Pointer variants.
  template <typename T>
  static T* AtomicAcquireLoadPtr(T* volatile* ptr) {
    return __atomic_load_n(ptr, __ATOMIC_ACQUIRE);
  }
  template <typename T>
  static T* AtomicCompareAndSwapPtr(T* volatile* ptr, T* old_value,
                                    T* new_value) {
    return __sync_val_compare_and_swap(ptr, old_value, new_value);
  }
#endif
};

#define AtomicTryLock(lock) (*(lock) == 0 && AtomicCmpSet(lock, 0, 1))
#define AtomicUnLock(lock) *(lock) = 0

uint32_t refcount_incr(uint32_t* refcount);
uint32_t refcount_decr(uint32_t* refcount);

}  // namespace MSF
#endif