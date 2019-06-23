/**************************************************************************
*
* Copyright (c) 2017-2019, luotang.me <wypx520@gmail.com>, China.
* All rights reserved.
*
* Distributed under the terms of the GNU General Public License v2.
*
* This software is provided 'as is' with no explicit or implied warranties
* in respect of its properties, including, but not limited to, correctness
* and/or fitness for purpose.
*
**************************************************************************/
#ifndef __MSF_MEM_H__
#define __MSF_MEM_H__

#include <msf_list.h>
#include <msf_file.h>
#include <msf_network.h>
#include <msf_atomic.h>
#include <msf_thread.h>
#include <msf_os.h>

/*debian-ubuntu:apt-get install libnuma-dev*/
#include <numa.h>
#include <numaif.h>
#include <numacompat1.h>

#include <sys/mman.h>
#include <sys/syscall.h>

#define HUGE_PAGE_SZ    (2*1024*1024)

#define msf_sfree(ptr) do {     \
        if ((ptr) != NULL) {    \
            free((ptr));        \
            (ptr) = NULL;}      \
        } while(0) 


/*
 *  Determine whether some value is a power of two, where zero is
 * *not* considered a power of two.
 */
static inline __attribute__((const))
s32 is_power_of_2(unsigned long n) {
    return (n != 0 && ((n & (n - 1)) == 0));
}


s32 msf_memalign(void **memptr, size_t alignment, size_t size);
void *msf_mmap(size_t length);
s32 msf_munmap(void *addr, size_t length);
void *msf_numa_alloc_onnode(size_t size, s32 node);
void msf_numa_free(void *start, size_t size);

s32 msf_numa_node_of_cpu(s32 cpu);
s32 msf_numa_run_on_node(s32 node);
s32 msf_pin_to_node(s32 cpu);
void *msf_malloc_huge_pages(size_t size);
void msf_free_huge_pages(void *ptr);
void *msf_numa_alloc(size_t size);
void *msf_numa_alloc_page(size_t bytes, s32 node);

void msf_numa_free_ptr(void *ptr);

/********************Interface NUMA**************************************/
s32 numa_node_to_cpusmask(int node, u64 *cpusmask, s32 *nr);

void msf_numa_tonode_memory(void *mem, size_t size, s32 node);


#endif
