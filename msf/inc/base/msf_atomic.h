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
#ifndef __MSF_ATOMIC_H__
#define __MSF_ATOMIC_H__

#include <msf_utils.h>

#if (MSF_HAVE_GCC_ATOMIC)
/* GCC 4.1 builtin atomic operations */
typedef volatile u32  msf_atomic_t;

#define msf_atomic_cmp_set(lock, old, set)  __sync_bool_compare_and_swap(lock, old, set)
#define msf_atomic_add(value, add)          __sync_add_and_fetch(value, add)
#define msf_atomic_fetch_add(value, add)    __sync_fetch_and_add(value, add)
#define msf_atomic_fetch_sub(value, sub)    __sync_fetch_and_sub(value, sub)
#define msf_atomic_sub(value, sub)          __sync_sub_and_fetch(value, sub)
#define msf_atomic_get(value,dstvar) do { \
    dstvar = __sync_sub_and_fetch(value, 0); \
} while(0)

#define msf_memory_barrier()                __sync_synchronize()

#if ( __i386__ || __i386 || __amd64__ || __amd64 )
#define msf_cpu_pause()         __asm__("pause")
#else
#define msf_cpu_pause()
#endif

#elif ( __amd64__ || __amd64 )

#define MSF_SMP_LOCK  "lock;"/
#define msf_memory_barrier()    __asm__ volatile ("" ::: "memory")
#define msf_cpu_pause()         __asm__ ("pause")

//#define msf_cpu_pause()       _mm_pause();

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

inline msf_atomic_uint_t msf_atomic_cmp_set(msf_atomic_t *lock,
    msf_atomic_uint_t old, msfatomic_uint_t set) {
    
    u8  res;
    __asm__ volatile (
         MSF_SMP_LOCK
    "    cmpxchgq  %3, %1;   "
    "    sete      %0;       "
    : "=a" (res) : "m" (*lock), "a" (old), "r" (set) : "cc", "memory");

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
inline msf_atomic_int_t msf_atomic_fetch_add(msf_atomic_t *value,
    msf_atomic_int_t add) {

    __asm__ volatile (
         MSF_SMP_LOCK
    "    xaddq  %0, %1;   "
    : "+r" (add) : "m" (*value) : "cc", "memory");
    return add;
}

# elif ( __i386__ || __i386 ) /* x86*/

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

inline msf_atomic_uint_t msf_atomic_cmp_set(msf_atomic_t *lock,
    msf_atomic_uint_t old, msf_atomic_uint_t set) {

    u8  res;
    __asm__ volatile (
         MSF_SMP_LOCK
    "    cmpxchgl  %3, %1;   "
    "    sete      %0;       "
    : "=a" (res) : "m" (*lock), "a" (old), "r" (set) : "cc", "memory");

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


#if !(( __GNUC__ == 2 && __GNUC_MINOR__ <= 7 ) || ( __INTEL_COMPILER >= 800 ))

/*
 * icc 8.1 and 9.0 compile broken code with -march=pentium4 option:
 * ngx_atomic_fetch_add() always return the input "add" value,
 * so we use the gcc 2.7 version.
 *
 * icc 8.1 and 9.0 with -march=pentiumpro option or icc 7.1 compile
 * correct code.
 */

inline msf_atomic_int_t
msf_atomic_fetch_add(msf_atomic_t *value, msf_atomic_int_t add)
{
    __asm__ volatile (

         MSF_SMP_LOCK
    "    xaddl  %0, %1;   "

    : "+r" (add) : "m" (*value) : "cc", "memory");

    return add;
}

#else

/*
 * gcc 2.7 does not support "+r", so we have to use the fixed
 * %eax ("=a" and "a") and this adds two superfluous instructions in the end
 * of code, something like this: "mov %eax, %edx / mov %edx, %eax".
 */

inline msf_atomic_int_t
msf_atomic_fetch_add(msf_atomic_t *value, msf_atomic_int_t add)
{
    msf_atomic_uint_t  old;

    __asm__ volatile (
        MSF_SMP_LOCK
        "    xaddl  %2, %1;   "

        : "=a" (old) : "m" (*value), "a" (add) : "cc", "memory");

    return old;
}

#endif


/*
 * on x86 the write operations go in a program order, so we need only
 * to disable the gcc reorder optimizations
 */

#define msf_memory_barrier()    __asm__ volatile ("" ::: "memory")
/* old "as" does not support "pause" opcode */
#define msf_cpu_pause()         __asm__ (".byte 0xf3, 0x90")

#elif !defined(__ATOMIC_VAR_FORCE_SYNC_MACROS) && defined(__ATOMIC_RELAXED) && !defined(__sun) && (!defined(__clang__) || !defined(__APPLE__) || __apple_build_version__ > 4210057)
/* Implementation using __atomic macros. */
#define msf_atomic_fetch_add(var,count) __atomic_add_fetch(&var,(count),__ATOMIC_RELAXED)
#define msf_atomic_add(var,oldvalue_var,count) do { \
    oldvalue_var = __atomic_fetch_add(&var,(count),__ATOMIC_RELAXED); \
} while(0)
#define msf_atomic_fetch_sub(var,count) __atomic_sub_fetch(&var,(count),__ATOMIC_RELAXED)
#define msf_atomic_get(var,dstvar) do { \
    dstvar = __atomic_load_n(&var,__ATOMIC_RELAXED); \
} while(0)

#define msf_atomic_cmp_set(var,value) __atomic_store_n(&var,value,__ATOMIC_RELAXED)

#endif

#define msf_trylock(lock)  (*(lock) == 0 && msf_atomic_cmp_set(lock, 0, 1))
#define msf_unlock(lock)    *(lock) = 0


u32 refcount_incr(u32 *refcount);
u32 refcount_decr(u32 *refcount);

#endif



