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
	RTLD_LAZY 	暂缓决定,等有需要时再解出符号 
　　RTLD_NOW 	立即决定,返回前解除所有未决定的符号 
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

#ifndef _UTILS_H_
#define _UTILS_H_

#include <msf_config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <stddef.h>
#include <ctype.h> /* uppper()*/
#include <time.h>
#include <limits.h>
#include <dlfcn.h>
#include <errno.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <pthread.h>

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

#define MSF_FUNC_LINE __FUNCTION__, __LINE__

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


#if (__FreeBSD__) && (__FreeBSD_version < 400017)

#include <sys/param.h>          /* ALIGN() */

/*
 * FreeBSD 3.x has no CMSG_SPACE() and CMSG_LEN() and has the broken CMSG_DATA
()
 */

#undef  CMSG_SPACE
#define CMSG_SPACE(l)       (ALIGN(sizeof(struct cmsghdr)) + ALIGN(l))

#undef  CMSG_LEN
#define CMSG_LEN(l)         (ALIGN(sizeof(struct cmsghdr)) + (l))

#undef  CMSG_DATA
#define CMSG_DATA(cmsg)     ((u8*)(cmsg) + ALIGN(sizeof(struct cmsghdr)))

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

#define MSF_THREAD_NAME(name)  prctl(PR_SET_NAME, name) 

/*
 * 当前进程仍然处于可执行状态,但暂时"让出"处理器,
 * 使得处理器优先调度其他可执行状态的进程,这样,
 * 在进程被内核再次调度时,在for循环代码中可以期望其他进程释放锁
 * 注意,不同的内核版本对于ngx_sched_yield系统调用的实现可能是不同的,
 * 但它们的目的都是暂时"让出"处理器*/
 
#if (MSF_HAVE_SCHED_YIELD)
#define msf_sched_yield()  sched_yield()
#else
#define msf_sched_yield()  usleep(1)
#endif

/* see /usr/src/kernel/linux/jiffies.h */
/* extern unsigned long volatile jiffies; */

#define jiffies     ((long)times(NULL))


/* Linux dl API*/
#ifndef WIN32  
#define MSF_DLHANDLE void*
#define MSF_DLOPEN_L(name) dlopen((name), RTLD_LAZY | RTLD_GLOBAL)
#define MSF_DLOPEN_N(name) dlopen((name), RTLD_NOW)


#if defined(WIN32)
#define MSF_DLSYM(handle, symbol)   GetProcAddress((handle), (symbol))
#define MSF_DLCLOSE(handle)         FreeLibrary(handle)
#else
#define MSF_DLSYM(handle, symbol)   dlsym((handle), (symbol))
#define MSF_DLCLOSE(handle)         dlclose(handle)
#endif
#define MSF_DLERROR() dlerror()
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

#define MSF_PACKED_MEMORY  __attribute__((__packed__))
#define MSF_ALWAYS_INLINE inline __attribute__((always_inline))
#define MSF_LIBRARY_INITIALIZER(f) \
    static void f(void)__attribute__((constructor)); \
    static void f(void)

#define MSF_LIBRARY_FINALIZER(f) \
    static void f(void)__attribute__((destructor)); \
    static void f(void)

static inline u64 msf_get_current_thread_id(void) {
    return (u64)pthread_self();
}


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

#define MSF_FILE_READ       0
#define MSF_FILE_WRITE      1
#define MSF_FILE_EXEC       2

struct file_info {
    u32     size;
    time_t  last_modification;

    /* Suggest flags to open this file */
    u32     flags_read_only;

    u32     exists;
    u32     is_file;
    u32     is_link;
    u32     is_directory;
    u32     exec_access;
    u32     read_access;
} __attribute__((__packed__));

typedef void (*sighandler_t)(s32);

s32 signal_handler(s32 sig, sighandler_t handler);
s32 sem_wait_i(sem_t* psem, s32 mswait);

s32 file_get_info(const s8 *path, struct file_info *f_info, u32 mode);

s8 *config_read_file(const s8 *filename);

void *plugin_load_dynamic(const s8 *path);

void *plugin_load_symbol(void *handler, const s8 *symbol);

s32 daemonize(s32 nochdir, s32 noclose);

s32 check_file_exist(s8 *file);

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

void save_pid(const s8 *pid_file);
void remove_pidfile(const s8 *pid_file);

u32 refcount_incr(u32 *refcount);
u32 refcount_decr(u32 *refcount);


s32 pthread_spawn(pthread_t *tid, void *(*func)(void *), void *arg);

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
    //
    return 0;
}

/* Notice: var pointer is need */
#define MSF_SWAP(a, b) {    \
    *a = *a ^ *b;           \
    *b = *a ^ *b;           \
    *a = *a ^ *b;           \
}

#endif
