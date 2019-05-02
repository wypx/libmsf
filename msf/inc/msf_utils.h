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

/*  Open Shared Object
    RTLD_LAZY   暂缓决定,等有需要时再解出符号 
　　RTLD_NOW  立即决定,返回前解除所有未决定的符号 
　　RTLD_LOCAL  与RTLD_GLOBAL作用相反,
                动态库中定义的符号不能被其后打开的其它库重定位
                如果没有指明是RTLD_GLOBAL还是RTLD_LOCAL,则缺省为RTLD_LOCAL.
　　RTLD_GLOBAL 允许导出符号  动态库中定义的符号可被其后打开的其它库重定位
　　RTLD_GROUP 
　　RTLD_WORLD 
    RTLD_NODELETE:在dlclose()期间不卸载库,
                并且在以后使用dlopen()重新加载库时不初始化库中的静态变量
                这个flag不是POSIX-2001标准。 
    RTLD_NOLOAD:不加载库
                可用于测试库是否已加载(dlopen()返回NULL说明未加载,
                否则说明已加载）,也可用于改变已加载库的flag
                如:先前加载库的flag为RTLD＿LOCAL
                用dlopen(RTLD_NOLOAD|RTLD_GLOBAL)后flag将变成RTLD_GLOBAL
                这个flag不是POSIX-2001标准
    RTLD_DEEPBIND:在搜索全局符号前先搜索库内的符号,避免同名符号的冲突
                这个flag不是POSIX-2001标准。     
*/
#ifndef _MSF_UTILS_H_
#define _MSF_UTILS_H_

#include <msf_config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <stddef.h>
#include <ctype.h> /* uppper()*/
#include <time.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <semaphore.h>

#define true    1
#define false   0

typedef char                    s8;
typedef short int               s16;
typedef int                     s32;

# if __WORDSIZE == 64  
typedef long int                s64;
# else
__extension__
typedef long long int           s64;
# endif


/* Unsigned.  */
typedef unsigned char           u8;
typedef unsigned short int      u16;
typedef unsigned int            u32;
 
#if __WORDSIZE == 64  
typedef unsigned long int       u64;
#else  
__extension__  
typedef unsigned long long int  u64;
#endif

#define MSF_NO_WAIT             0
#define MSF_WAIT_FOREVER       -1

#define MSF_CONF_UNSET_UINT  (u32) -1

#define MSF_INT32_LEN   (sizeof("-2147483648") - 1)
#define MSF_INT64_LEN   (sizeof("-9223372036854775808") - 1)

#define MSF_FUNC_FILE_LINE __func__, __FILE__, __LINE__

/* If supported, give compiler hints for branch prediction. */
#if !defined(__GNUC__) || (__GNUC__ == 2 && __GNUC_MINOR__ < 96)
#define __builtin_expect(x, expected_value) (x)
#endif

/* linux-2.6.38.8/include/linux/compiler.h */  
# define likely(x)      __builtin_expect(!!(x), 1)  
# define unlikely(x)    __builtin_expect(!!(x), 0)

/* linux-2.6.38.8/include/linux/compiler.h */  
# ifndef likely  
#  define likely(x)     (__builtin_constant_p(x) ? !!(x) : __branch_check__(x, 1))  
# endif  
# ifndef unlikely  
#  define unlikely(x)   (__builtin_constant_p(x) ? !!(x) : __branch_check__(x, 0))  
# endif

#if __GNUC__ > 2
#define NORETURN __attribute__ ((__noreturn__))
#else
#define NORETURN
#endif

/* linux/kernel.h
 * min()/max() macros that also do
 * strict type-checking.. See the
 * "unnecessary" pointer comparison.
 */
#define min(x,y) ({         \
    typeof(x) _x = (x);     \
    typeof(y) _y = (y);     \
    (void) (&_x == &_y);    \
    _x < _y ? _x : _y; })

#define max(x,y) ({         \
    typeof(x) _x = (x);     \
    typeof(y) _y = (y);     \
    (void) (&_x == &_y);    \
    _x > _y ? _x : _y; })

#ifndef roundup
# define roundup(x, y)  ((((x) + ((y) - 1)) / (y)) * (y))
#endif /* !defined(roundup) */

#define MSF_ABS(value)       (((value) >= 0) ? (value) : - (value))

#define MSF_NEW(t,n)        ((t*) malloc(sizeof(t) * (n)))
#define MSF_RENEW(p,t,n)    ((t*) realloc((void*) p, sizeof(t) * (n)))
                     
#define MSF_LOBYTE(a)       (BYTE)(a)
#define MSF_HIBYTE(a)       (BYTE)((a)>>8)
#define MSF_LOWORD(a)       (WORD)(a)
#define MSF_HIWORD(a)       (WORD)((a)>>16)
#define MSF_MAKEWORD(a,b)   (WORD)(((a)&0xff)|((b)<<8))
#define MSF_MAKELONG(a,b) 	(DWORD)(((a)&0xffff)|((b)<<16))

#ifndef BIT_SET
#define BIT_SET(v,f)        (v) |= (f)
#endif

#ifndef BIT_CLEAR
#define BIT_CLEAR(v,f)      (v) &= ~(f)
#endif

#ifndef BIT_IS_SET
#define BIT_IS_SET(v,f)     ((v) & (f))
#endif

#include <sys/types.h>
#include <byteswap.h>
#include <endian.h>

#if __BYTE_ORDER == __BIG_ENDIAN
#define cpu_to_le16(x)      bswap_16(x)
#define le16_to_cpu(x)      bswap_16(x)
#define cpu_to_le32(x)      bswap_32(x)
#define le32_to_cpu(x)      bswap_32(x)
#define cpu_to_be16(x)      (x)
#define be16_to_cpu(x)      (x)
#define cpu_to_be32(x)      (x)
#define be32_to_cpu(x)      (x)
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#define cpu_to_le16(x)      (x)
#define le16_to_cpu(x)      (x)
#define cpu_to_le32(x)      (x)
#define le32_to_cpu(x)      (x)
#define cpu_to_be16(x)      bswap_16(x)
#define be16_to_cpu(x)      bswap_16(x)
#define cpu_to_be32(x)      bswap_32(x)
#define be32_to_cpu(x)      bswap_32(x)
#else
#error "unknown endianess!"
#endif

#ifndef _ARRAY_SIZE
#define _ARRAY_SIZE(x) (sizeof (x) / sizeof (x[0]))
#endif

#define UNUSED(expr) do { (void)(expr); } while (0)

/* Do overlapping strcpy safely, by using memmove. */
#define msf_ol_strcpy(dst,src) memmove(dst, src, strlen(src)+1)

#ifndef va_copy
#ifdef __va_copy
#define va_copy __va_copy
#else
#define va_copy(x, y) (x) = (y)
#endif
#endif

#define MSF_NEW(t,n)        ((t*) malloc(sizeof(t) * (n)))
#define MSF_RENEW(p,t,n)    ((t*) realloc((void*) p, sizeof(t) * (n)))

#define sfree(ptr) do { \
        if ((ptr) != NULL) { \
            free((ptr)); \
            (ptr) = NULL;} \
        } while(0) 

#define sclose(fd) do { \
    u32 ret = close(fd); \
    if (fd != -1) { \
        while (ret != 0) { \
            if (errno != EINTR || errno == EBADF) \
                break; \
            ret = close(fd); \
        } \
        (fd) = -1;} \
    } while(0)

#define sfclose(fp) do { \
    if ((fp) != NULL) { \
        fclose((fp)); \
        (fp) = NULL;} \
    } while(0)

/* 这个表示一个方法的返回值只由参数决定, 如果参数不变的话,
 *  就不再调用此函数，直接返回值.经过我的尝试发现还是调用了,
 *  后又经查资料发现要给gcc加一个-O的参数才可以.
 *  是对函数调用的一种优化*/
#define MSF_CONST_CALL __attribute__((const))
/* 表示函数的返回值必须被检查或使用,否则会警告*/
#define MSF_UNUSED_CHECK __attribute__((unused))
#define MSF_PACKED_MEMORY  __attribute__((__packed__))
/* Force compiler to use inline*/
#define MSF_ALWAYS_INLINE inline __attribute__((always_inline))
#define MSF_LIBRARY_INITIALIZER(func, level) \
    static void func(void)__attribute__((constructor(level))); \
    static void func(void)

#define MSF_LIBRARY_FINALIZER(func) \
    static void func(void)__attribute__((destructor)); \
    static void func(void)

/** when static library not been linked, 
  * this check is needed.
  */
#ifndef ATTRI_CHECK
#define MSF_ATTRIBUTE_WIKE  __attribute__((weak))
#else
#define MSF_ATTRIBUTE_WIKE
#endif

#define msf_tolower(c)      (u8) ((c >= 'A' && c <= 'Z') ? (c | 0x20) : c)
#define msf_toupper(c)      (u8) ((c >= 'a' && c <= 'z') ? (c & ~0x20) : c)

#define msf_strncmp(s1, s2, n)  strncmp((const char *) s1, (const char *) s2, n)


/* msvc and icc7 compile strcmp() to inline loop */
#define msf_strcmp(s1, s2)  strcmp((const s8 *) s1, (const s8 *) s2)

#define msf_strstr(s1, s2)  strstr((const s8 *) s1, (const s8 *) s2)
#define msf_strlen(s)       strlen((const s8 *) s)

#define msf_strchr(s1, c)   strchr((const s8 *) s1, (s32) c)

/*
 * msvc and icc7 compile memset() to the inline "rep stos"
 * while ZeroMemory() and bzero() are the calls.
 * icc7 may also inline several mov's of a zeroed register for small blocks.
 */
#define msf_memzero(buf, n)       (void) memset(buf, 0, n)
#define msf_memset(buf, c, n)     (void) memset(buf, c, n)

/** Replacement for offsetof on platforms that don't define it. */
#ifndef offsetof
#define offsetof(type, field) ((off_t)(&((type *)0)->field))
#endif

#define msf_align(d, a)         (((d) + (a - 1)) & ~(a - 1))

#define msf_align_ptr(p, a)  \
    (u8 *) (((u32) (p) + ((u32) a - 1)) & ~((u32) a - 1))

#define msf_test_bits(mask, addr)   (((*addr) & (mask)) != 0)
#define msf_clr_bits(mask, addr)    ((*addr) &= ~(mask))
#define msf_set_bits(mask, addr)    ((*addr) |= (mask))

#define msf_test_bit(nr, addr)  (*(addr) & (1ULL << (nr)))
#define msf_set_bit(nr, addr)   (*(addr) |=  (1ULL << (nr)))
#define msf_clr_bit(nr, addr)   (*(addr) &=  ~(1ULL << (nr)))

typedef void (*sighandler_t)(s32);

s32 signal_handler(s32 sig, sighandler_t handler);
s32 sem_wait_i(sem_t* psem, s32 mswait);

s32 daemonize(s32 nochdir, s32 noclose);

/*
 * Wrappers around strtoull/strtoll that are safer and easier to
 * use.  For tests and assumptions, see internal_tests.c.
 *
 * str   a NULL-terminated base decimal 10 unsigned integer
 * out   out parameter, if conversion succeeded
 *
 * returns true if conversion succeeded.
 */
u8 safe_strtoull(const s8 *str, u64 *out);
u8 safe_strtoll(const s8 *str, s64 *out);
u8 safe_strtoul(const s8 *str, u32 *out);
u8 safe_strtol(const s8 *str, s32 *out);

u32 refcount_incr(u32 *refcount);
u32 refcount_decr(u32 *refcount);

inline s32 get_value()
{
 //_sysnc_fetch_and_add
    //多线程原子操作 无所操作
    //read_lock
    //_bittset test_bit
    //atuomoc_read
    //automic_des/inc
    //rwlock_t
    //mm-segment_t fs = get_fs() set_fs_get_ds())
    //kmalloc vmalooc 
    //semphone

    //ipref带宽丢包抖动 netpref qpref时延
    //vnsatat
    //sar监控
    //mleak内存监控
    //0拷贝 节省用户态到内核态的拷贝
    //分布式存储解决方案
    //system/popen命令注入风险 execve代替
    //有= %#8x
    //sparseg工具
    //_rcu
    //hping

    return 0;
}

/* Notice: var pointer is need */
#define MSF_SWAP(a, b) {    \
    *a = *a ^ *b;           \
    *b = *a ^ *b;           \
    *a = *a ^ *b;           \
}

#endif
