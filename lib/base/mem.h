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
#ifndef BASE_MEM_H_
#define BASE_MEM_H_

#include <malloc.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
// #include <netinet/tcp.h>
/*debian-ubuntu:apt-get install libnuma-dev*/
#include <numa.h>
#include <numacompat1.h>
#include <numaif.h>

#include "bitops.h"
#include "gcc_attr.h"

using namespace MSF;

namespace MSF {

#define MSF_HUGE_PAGE_SIZE (2 * 1024 * 1024)

#define MSF_NEW(t, n) ((t *)malloc(sizeof(t) * (n)))
#define MSF_RENEW(p, t, n) ((t *)realloc((void *)p, sizeof(t) * (n)))
#define MSF_NEWCLR(num, t, n) ((t *)calloc(num, sizeof(t) * n))
#define MSF_MALLOC_TYPE(t, n) ((t *)malloc(n))
#define MSF_CALLOC_TYPE(num, t, n) ((t *)calloc(num, n))

/*
 * msvc and icc7 compile memset() to the inline "rep stos"
 * while ZeroMemory() and bzero() are the calls.
 * icc7 may also inline several mov's of a zeroed register for small blocks.
 */
#define MSF_MEMZERO(buf, n) (void)memset(buf, 0, n)
#define MSF_MEMSET(buf, c, n) (void)memset(buf, c, n)
#define MSF_MEMCPY(dst, src, n) memcpy(dst, src, n)
#define MSF_MEMMOOVE(dst, src, n) memmove(dst, src, n)

#define MSF_FREE_S(ptr)  \
  do {                   \
    if ((ptr) != NULL) { \
      free((ptr));       \
      (ptr) = NULL;      \
    }                    \
  } while (0)

// void * BasicMemMap(size_t length);

// inline void* BasicAlloc(size_t n);
// inline void BasicFree(void* p, size_t n);

// inline void* BasicAlignedAlloc(size_t size, size_t align);
// inline void BasicAlignedFree(void* aligned_ptr);

inline void *BasicMemMap(size_t length) {
  return mmap(NULL, length, PROT_READ | PROT_WRITE,
              MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE | MAP_HUGETLB, -1, 0);
}

inline void *BasicAlloc(size_t n) { return ::operator new(n); }

inline void BasicFree(void *p, size_t n) {
#if __cpp_sized_deallocation
  return ::operator delete(p, n);
#else
  (void)n;
  return ::operator delete(p);
#endif
}

#if _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600 ||                    \
    (defined(__ANDROID__) && (__ANDROID_API__ > 16)) ||                      \
    (defined(__APPLE__) && (__MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_10_6 || \
                            __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_3_0))

inline void *BasicAlignedAlloc(size_t size, size_t align) {
  // use posix_memalign, but mimic the behaviour of memalign
  void *ptr = nullptr;
  int32_t rc = posix_memalign(&ptr, align, size);
  return rc == 0 ? (errno = 0, ptr) : (errno = rc, nullptr);
}

inline void BasicAlignedFree(void *aligned_ptr) { free(aligned_ptr); }

#elif defined(_WIN32)

inline void *BasicAlignedAlloc(size_t size, size_t align) {
  return ::_aligned_malloc(size, align);
}
inline void BasicAlignedFree(void *aligned_ptr) {
  ::_aligned_free(aligned_ptr);
}

#else

inline void *BasicAlignedAlloc(size_t size, size_t align) {
  return ::memalign(align, size);
}

inline void BasicAlignedFree(void *aligned_ptr) { ::free(aligned_ptr); }
#endif

void *BasicNumaAllocOnNode(size_t size, int node);
void *BasicNumaAllocLocal(size_t size);
void *BasicNumaAllocInterleaved(size_t size);
void *BasicNumaAlloc(size_t size);
void BasicNumaFree(void *start, size_t size);
int BasicNumaPeferred(void);
void BasicNumaSetLocalAlloc(void);
int BasicNumaNodeOfCpu(int cpu);
int BasicNumaRunOnNode(int node);
int BasicNumaNumTaskCpus(void);
int BasicNumaNumTaskNodes(void);
int BasicNumaNumConfNodes(void);
int BasicNumaNumConfCpus(void);
int BasicNumaNumPossibleNodes(void);
int BasicNumaNumPossibleCpus(void);
int BasicNumaPinNode(int cpu);
void *BaseAllocHugePages(size_t size);
void BasicFreeHugePages(void *ptr);
void *BaseNumaAllocPages(size_t bytes, int node);
void BaseNumaFreePages(void *ptr);
/********************Interface NUMA**************************************/
int numa_node_to_cpusmask(int node, uint64_t *cpusmask, int *nr);
void msf_numa_tonode_memory(void *mem, size_t size, int node);
long long msf_numa_node_size(int node, long long *freep);
int msf_numa_distance(int node1, int node2);

//! Total RAM in bytes
int64_t RamTotal();
//! Free RAM in bytes
int64_t RamFree();

}  // namespace MSF
#endif