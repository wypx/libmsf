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
#include <msf_mem.h>

static struct msf_os *g_os = NULL;

MSF_LIBRARY_INITIALIZER(msf_mem_init, 101) {
    g_os = msf_get_os();
}

s32 msf_memalign(void **memptr, size_t alignment, size_t size) {
    return posix_memalign(memptr, alignment, size);
}

void *msf_mem_mmap(size_t length) {
    return mmap(NULL, length, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS |
            MAP_POPULATE | MAP_HUGETLB, -1, 0);
}

s32 msf_mem_munmap(void *addr, size_t length) {
    return munmap(addr, length);
}


void *msf_numa_alloc_onnode(size_t size, s32 node) {
    return numa_alloc_onnode(size, node);
}

void *msf_numa_alloc_local(size_t size) {
    return numa_alloc_local(size);
}

void *msf_numa_alloc_interleaved(size_t size) {
    return numa_alloc_interleaved(size);
}

void *msf_numa_alloc(size_t size) {
    return numa_alloc(size);
}

void msf_numa_free(void *start, size_t size) {
    numa_free(start, size);
}

s32 msf_numa_preferred(void) {
    return numa_preferred();
}

void msf_numa_set_localalloc(void) {
    numa_set_localalloc();
}

s32 msf_numa_node_of_cpu(s32 cpu) {
    return numa_node_of_cpu(cpu);
}

s32 msf_numa_run_on_node(s32 node) {
    return numa_run_on_node(node);
}

s32 msf_numa_num_task_cpus(void) {
    return numa_num_task_cpus();
}

s32 msf_numa_num_task_nodes(void) {
    return numa_num_task_nodes();
}

s32 msf_numa_num_configured_nodes(void) {
    return numa_num_configured_nodes();
}

s32 msf_numa_num_configured_cpus(void) {
    return numa_num_configured_cpus();
}

s32 msf_numa_num_possible_nodes(void) {
    return numa_num_possible_nodes();
}

s32 msf_numa_num_possible_cpus(void) {
    return numa_num_possible_cpus();
}



s32 msf_pin_to_node(s32 cpu) {
    s32 node = msf_numa_node_of_cpu(cpu);
    /* pin to node */
    s32 ret = msf_numa_run_on_node(node);

    if (ret)
        return -1;

    /* is numa_run_on_node() guaranteed to take effect immediately? */
    msf_sched_yield();

    return -1;
}

void *msf_malloc_huge_pages(size_t size) {
    s32 retval;
    size_t  real_size;
    void    *ptr = NULL;

#ifndef WIN32
    long page_size = g_os->pagesize;

    if (page_size < 0) {
        printf("sysconf failed. (errno=%d %m)\n", errno);
        return NULL;
    }

    real_size = MSF_ALIGN(size, page_size);
    retval = msf_memalign(&ptr, page_size, real_size);
    if (retval) {
        printf("posix_memalign failed sz:%zu. %s\n",
             real_size, strerror(retval));
        return NULL;
    }
    memset(ptr, 0, real_size);
    return ptr;
#endif
    /* Use 1 extra page to store allocation metadata */
    /* (libhugetlbfs is more efficient in this regard) */
    real_size = MSF_ALIGN(size + HUGE_PAGE_SZ, HUGE_PAGE_SZ);

    ptr = msf_mem_mmap(real_size);
    if (!ptr || ptr == MAP_FAILED) {
        /* The mmap() call failed. Try to malloc instead */
        long page_size = g_os->pagesize;

        if (page_size < 0) {
             printf("sysconf failed. (errno=%d %m)\n", errno);
            return NULL;
        }
        printf("huge pages allocation failed, allocating " \
             "regular pages\n");

         printf("mmap rdma pool sz:%zu failed (errno=%d %m)\n",
          real_size, errno);
        real_size = MSF_ALIGN(size + HUGE_PAGE_SZ, page_size);
        retval = msf_memalign(&ptr, page_size, real_size);
        if (retval) {
             printf("posix_memalign failed sz:%zu. %s\n",
              real_size, strerror(retval));
            return NULL;
        }
        memset(ptr, 0, real_size);
        real_size = 0;
    } else {
        printf("Allocated huge page sz:%zu\n", real_size);
    }
    /* Save real_size since mmunmap() requires a size parameter */
    *((size_t *)ptr) = real_size;
    /* Skip the page with metadata */
    return (s8*)ptr + HUGE_PAGE_SZ;
}


void msf_free_huge_pages(void *ptr) {
    void    *real_ptr;
    size_t  real_size;

    if (!ptr)
        return;

#ifndef WIN32
        sfree(ptr);
        return;
#endif

    /* Jump back to the page with metadata */
    real_ptr = (s8 *)ptr - HUGE_PAGE_SZ;
    /* Read the original allocation size */
    real_size = *((size_t *)real_ptr);

    if (real_size != 0)
        /* The memory was allocated via mmap()
           and must be deallocated via munmap()
           */
        msf_mem_munmap(real_ptr, real_size);
    else
        /* The memory was allocated via malloc()
           and must be deallocated via free()
           */
        sfree(real_ptr);
}


void *msf_numa_alloc_page(size_t bytes, s32 node) {

    if (g_os->en_numa < 0) { 
        return NULL;
    } 

    long page_size = g_os->pagesize;
    size_t real_size = MSF_ALIGN((bytes + page_size), page_size);
    void *p = msf_numa_alloc_onnode(real_size, node);

    if (!p) {
        printf("numa_alloc_onnode failed sz:%zu. %m\n",
              real_size);
        return NULL;
    }
    /* force the OS to allocate physical memory for the region */
    msf_memzero(p, real_size);

    /* Save real_size since numa_free() requires a size parameter */
    *((size_t *)p) = real_size;

    /* Skip the page with metadata */
    return (s8*)p + page_size;
}

void msf_numa_free_ptr(void *ptr) {

    void    *real_ptr;
    size_t  real_size;

    if (!ptr) return;

    long page_size = g_os->pagesize;

    /* Jump back to the page with metadata */
    real_ptr = (s8 *)ptr - page_size;
    /* Read the original allocation size */
    real_size = *((size_t *)real_ptr);

    if (real_size != 0)
        /* The memory was allocated via numa_alloc()
           and must be deallocated via numa_free()
           */
        msf_numa_free(real_ptr, real_size);
    }


/********************Interface NUMA**************************************/
s32 numa_node_to_cpusmask(int node, u64 *cpusmask, s32 *nr) {
    struct bitmask *mask;
    u64    bmask = 0;
    s32 retval = -1;
    u32 i;

    mask = numa_allocate_cpumask();
    retval = numa_node_to_cpus(node, (unsigned long *)mask, sizeof(struct bitmask));
    if (retval < 0)
        goto cleanup;

    *nr = 0;
    for (i = 0; i < mask->size && i < 64; i++) {
        if (numa_bitmask_isbitset(mask, i)) {
            MSF_SET_BIT(i, &bmask);
            (*nr)++;
        }
    }

    retval = 0;
cleanup:
    *cpusmask = bmask;

    numa_free_cpumask(mask);
    return retval;
}


void msf_numa_tonode_memory(void *mem, size_t size, s32 node) {
    numa_tonode_memory(mem, size, node);
}

long msf_numa_node_size(s32 node, long * freep) {
    return numa_node_size(node, freep);
}

s32 msf_numa_distance(s32 node1, s32 node2) {
    return numa_distance(node1, node2);
}


