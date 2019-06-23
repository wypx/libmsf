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
#ifndef __MSF_MEMPOLL_H__
#define __MSF_MEMPOLL_H__

#include <msf_mem.h>

//#define DEBUG_MEMPOOL_MT    0

#define MSF_MAX_SLABS_NR    6
#define MSF_MEM_SLABS_NR    4

#define _16K_BLOCK_SZ       (16 * 1024)
#define _16K_MIN_NR         0
#define _16K_MAX_NR         (1024 * 24)
#define _16K_ALLOC_NR       128

#define _64K_BLOCK_SZ       (64 * 1024)
#define _64K_MIN_NR         0
#define _64K_MAX_NR         (1024 * 24)
#define _64K_ALLOC_NR       128

#define _256K_BLOCK_SZ      (256 * 1024)
#define _256K_MIN_NR        0
#define _256K_MAX_NR        (1024 * 24)
#define _256K_ALLOC_NR      128

#define _1M_BLOCK_SZ        (1024 * 1024)
#define _1M_MIN_NR          0
#define _1M_MAX_NR          (1024 * 24)
#define _1M_ALLOC_NR        128


/**
 * @enum xio_mempool_flag
 * @brief creation flags for mempool
 */
enum msf_mempool_flag {
    MSF_MEMPOOL_FLAG_NONE                   = 0x0000,
    MSF_MEMPOOL_FLAG_REG_MR                 = 0x0001,
    MSF_MEMPOOL_FLAG_HUGE_PAGES_ALLOC       = 0x0002,
    MSF_MEMPOOL_FLAG_NUMA_ALLOC             = 0x0004,
    MSF_MEMPOOL_FLAG_REGULAR_PAGES_ALLOC    = 0x0008,
    /**< do not allocate buffers from larger slabs,
     *   if the smallest slab is empty
     */
    MSF_MEMPOOL_FLAG_USE_SMALLEST_SLAB      = 0x0016
};


struct msf_mr {
    void        *addr;  /* for new devices */
    size_t      length; /* for new devices */
    s32         access; /* for new devices */
    s32         addr_alloced;   /* address was allocated by xio */
    struct list_head        dm_list;
    struct list_head        mr_list_entry;
} MSF_PACKED_MEMORY;

/**
 * @struct xio_reg_mem
 * @brief registered memory buffer descriptor
 *        used by all allocation and registration methods
 *        it's the user responsibility to save allocation type and use an
 *        appropriate free method appropriately
 */
struct msf_reg_mem {
    void            *addr;  /**< buffer's memory address     */
    size_t          length; /**< buffer's memory length */
    struct msf_mr   *mr;    /**< xio specific memory region    */
    void            *priv;  /**< xio private data     */
} MSF_PACKED_MEMORY;

struct msf_mem_block {
    struct msf_mem_slab     *parent_slab;
    struct msf_mr           *omr;

    void                    *buf;
    struct msf_mem_block    *next;
    msf_atomic_t             refcnt_claim;
    msf_atomic_t            refcnt;
    struct list_head        blocks_list_entry;
} MSF_PACKED_MEMORY;

struct msf_mem_region {
    struct msf_mr           *omr;
    void                    *buf;
    struct list_head        mem_region_entry;
} MSF_PACKED_MEMORY;

struct msf_mem_slab {

    struct msf_mempool      *pool;
    struct list_head        mem_regions_list;
    struct msf_mem_block    *free_blocks_list;
    struct list_head        blocks_list;

    size_t                  mb_size;    /*memory block size */
    pthread_spinlock_t      lock;

    s32                     init_mb_nr; /* initial mb size */
    s32                     curr_mb_nr; /* current size */
    s32                     max_mb_nr;  /* max allowed size */
    s32                     alloc_quantum_nr; /* number of items per allocation */
    msf_atomic_t            used_mb_nr;
    s32                     align;
    s32                     pad;
} MSF_PACKED_MEMORY;

struct msf_mempool {
    u32 slabs_nr; /* less sentinel */
    u32 flags;
    s32 nodeid;
    s32 safe_mt;
    struct msf_mem_slab *slab;
} MSF_PACKED_MEMORY;

struct msf_mempool_config {
    /**< number of slabs */
    size_t              slabs_nr;

    /**< per slab configuration */
    struct msf_mempool_slab_config {
        /**< slab's block memory size in bytes */
        size_t          block_sz;

        /**< initial number of allocated blocks */
        size_t          init_blocks_nr;
        /**< growing quantum of block allocations */
        size_t          grow_blocks_nr;
        /**< maximum number of allocated blocks */
        size_t          max_blocks_nr;
    } slab_cfg[MSF_MAX_SLABS_NR];
}MSF_PACKED_MEMORY;

s32 msf_mem_alloc(size_t length, struct msf_reg_mem *reg_mem);
s32 msf_mem_free(struct msf_reg_mem *reg_mem);

s32 msf_mempool_add_slab(struct msf_mempool *p,
        size_t size, size_t min, size_t max,
        size_t alloc_quantum_nr, s32 alignment);

struct msf_mempool * msf_mempool_create_prv(s32 id, u32 flag);
void msf_mempool_dump(struct msf_mempool *p);

void msf_mempool_destroy(struct msf_mempool *p);

s32 msf_mempool_alloc(struct msf_mempool *p, size_t length,
              struct msf_reg_mem *reg_mem);

void msf_mempool_free(struct msf_reg_mem *reg_mem);

#endif
