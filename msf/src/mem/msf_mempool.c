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
#include <msf_mempool.h>
#include <msf_os.h>

static struct msf_os *g_os = NULL;

MSF_LIBRARY_INITIALIZER(msf_mem_init, 101) {
    g_os = msf_get_os();
}


struct msf_mempool_config g_mempool_config = {

    MSF_MEM_SLABS_NR, {

        {_16K_BLOCK_SZ,  _16K_MIN_NR,  _16K_ALLOC_NR,  _16K_MAX_NR},
        {_64K_BLOCK_SZ,  _64K_MIN_NR,  _64K_ALLOC_NR,  _64K_MAX_NR},
        {_256K_BLOCK_SZ, _256K_MIN_NR, _256K_ALLOC_NR, _256K_MAX_NR},
        {_1M_BLOCK_SZ,   _1M_MIN_NR,   _1M_ALLOC_NR,   _1M_MAX_NR},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    }

};

s32 msf_mem_register(void *addr, size_t length, 
                        struct msf_reg_mem *reg_mem);
s32 msf_mem_dereg(struct msf_reg_mem *reg_mem);


/* Lock free algorithm based on: Maged M. Michael & Michael L. Scott's
 * Correction of a Memory Management Method for Lock-Free Data Structures
 * of John D. Valois's Lock-Free Data Structures. Ph.D. Dissertation
 */
static inline s32 decrement_and_test_and_set(msf_atomic_t *ptr)
{
    int old, _new;

    do {
    old  = *ptr;
    _new = old - 2;
    if (_new == 0)
        _new = 1; /* claimed be MP */
    } while (!msf_atomic_cmp_set(ptr, old, _new));

    return (old - _new) & 1;
}

static inline void clear_lowest_bit(msf_atomic_t *ptr)
{
    int old, _new;

    do {
        old = *ptr;
        _new = old - 1;
    } while (!msf_atomic_cmp_set(ptr, old, _new));
}

static inline void reclaim(struct msf_mem_slab *slab, struct msf_mem_block *p) {
    struct msf_mem_block *q;

    do {
        q = slab->free_blocks_list;
        p->next = q;
    } while (!msf_atomic_cmp_set(&slab->free_blocks_list, q, p));
}

static inline void safe_release(struct msf_mem_slab *slab,
            struct msf_mem_block *p)
{
    if (!p)
        return;

    if (decrement_and_test_and_set(&p->refcnt_claim) == 0)
        return;

    reclaim(slab, p);
}

static inline void non_safe_release(struct msf_mem_slab *slab,
             struct msf_mem_block *p)
{
    struct msf_mem_block *q;

    if (!p)
        return;

    q = slab->free_blocks_list;
    p->next = q;
    slab->free_blocks_list = p;
}

static struct msf_mem_block *safe_read(struct msf_mem_slab *slab)
{
    struct msf_mem_block *q;

    while (1) {
        q = slab->free_blocks_list;
        if (!q)
            return NULL;
        msf_atomic_fetch_add(&q->refcnt_claim, 2);
        /* make sure q is still the head */
            if (msf_atomic_cmp_set(&slab->free_blocks_list,
                     q, q))
                return q;
        safe_release(slab, q);
    }
}

static struct msf_mem_block *safe_new_block(struct msf_mem_slab *slab)
{
    struct msf_mem_block *p;

    while (1) {
        p = safe_read(slab);
        if (!p)
            return NULL;

        if (msf_atomic_cmp_set(&slab->free_blocks_list, p, p->next)) {
            clear_lowest_bit(&p->refcnt_claim);
            return p;
        }
        safe_release(slab, p);
    }
}

static struct msf_mem_block *non_safe_new_block(struct msf_mem_slab *slab)
{
    struct msf_mem_block *p;

    if (!slab->free_blocks_list)
        return NULL;

    p = slab->free_blocks_list;
    slab->free_blocks_list = p->next;
    p->next = NULL;

    return p;
}

/*---------------------------------------------------------------------------*/
/* msf_mem_slab_free							     */
/*---------------------------------------------------------------------------*/
static int msf_mem_slab_free(struct msf_mem_slab *slab)
{
    struct msf_mem_region *r, *tmp_r;

    slab->free_blocks_list = NULL;

#ifdef DEBUG_MEMPOOL_MT
    if (slab->used_mb_nr)
    printf("buffers are still in use before free: " \
      "pool:%p - slab[%p]: " \
      "size:%zd, used:%d, alloced:%d, max_alloc:%d\n",
      slab->pool, slab, slab->mb_size, slab->used_mb_nr,
      slab->curr_mb_nr, slab->max_mb_nr);
#endif

    if (slab->curr_mb_nr) {
        list_for_each_entry_safe(r, tmp_r, &slab->mem_regions_list,
                     mem_region_entry) {
            list_del(&r->mem_region_entry);
            if (MSF_TEST_BITS(MSF_MEMPOOL_FLAG_REG_MR,
                      &slab->pool->flags)) {
                struct msf_reg_mem   reg_mem;

                reg_mem.mr = r->omr;
                msf_mem_dereg(&reg_mem);
            }

            if (MSF_TEST_BITS(MSF_MEMPOOL_FLAG_HUGE_PAGES_ALLOC,
                       &slab->pool->flags))
                msf_free_huge_pages(r->buf);
            else if (MSF_TEST_BITS(MSF_MEMPOOL_FLAG_NUMA_ALLOC,
                       &slab->pool->flags))
                msf_numa_free_ptr(r->buf);
            else if (MSF_TEST_BITS(MSF_MEMPOOL_FLAG_REGULAR_PAGES_ALLOC,
                       &slab->pool->flags))
                sfree(r->buf);
            sfree(r);
        }
    }

    return 0;
}

/*---------------------------------------------------------------------------*/
/* msf_mem_slab_resize							     */
/*---------------------------------------------------------------------------*/
static struct msf_mem_block *msf_mem_slab_resize(struct msf_mem_slab *slab,
                    s32 alloc)
{
    s8                      *buf;
    struct msf_mem_region   *region;
    struct msf_mem_block    *block;
    struct msf_mem_block    *pblock;
    struct msf_mem_block    *qblock;
    struct msf_mem_block    dummy;
    s32                     nr_blocks;
    size_t                  region_alloc_sz;
    size_t                  data_alloc_sz;
    s32                     i;
    size_t                  aligned_sz;

    if (slab->curr_mb_nr == 0) {
        if (slab->init_mb_nr > slab->max_mb_nr)
            slab->init_mb_nr = slab->max_mb_nr;
        if (slab->init_mb_nr == 0)
            nr_blocks = min(slab->max_mb_nr,
                    slab->alloc_quantum_nr);
        else
            nr_blocks = slab->init_mb_nr;
    } else {
        nr_blocks =  slab->max_mb_nr - slab->curr_mb_nr;
        nr_blocks = min(nr_blocks, slab->alloc_quantum_nr);
    }
    if (nr_blocks <= 0)
        return NULL;

    region_alloc_sz = sizeof(*region) +
        nr_blocks * sizeof(struct msf_mem_block);
    buf = (s8 *)calloc(region_alloc_sz, sizeof(u8));
    if (!buf)
        return NULL;

    /* region */
    region = (struct msf_mem_region *)buf;
    buf = buf + sizeof(*region);
    block = (struct msf_mem_block *)buf;

    /* region data */
    aligned_sz = MSF_ALIGN(slab->mb_size, slab->align);
    data_alloc_sz = nr_blocks * aligned_sz;

    /* allocate the buffers and register them */
    if (MSF_TEST_BITS(MSF_MEMPOOL_FLAG_HUGE_PAGES_ALLOC,
              &slab->pool->flags))
        region->buf = msf_malloc_huge_pages(data_alloc_sz);
    else if (MSF_TEST_BITS(MSF_MEMPOOL_FLAG_NUMA_ALLOC,
               &slab->pool->flags))
        region->buf = msf_numa_alloc_page(data_alloc_sz, slab->pool->nodeid);
    else if (MSF_TEST_BITS(MSF_MEMPOOL_FLAG_REGULAR_PAGES_ALLOC,
               &slab->pool->flags)) {
        if(msf_memalign(&region->buf, slab->align, data_alloc_sz) !=0)
            region->buf = NULL;
    }

    if (!region->buf) {
        sfree(region);
        return NULL;
    }

    if (MSF_TEST_BITS(MSF_MEMPOOL_FLAG_REG_MR, &slab->pool->flags)) {
        struct msf_reg_mem reg_mem;

        msf_mem_register(region->buf, data_alloc_sz, &reg_mem);
        region->omr = reg_mem.mr;
        if (!region->omr) {
            if (MSF_TEST_BITS(MSF_MEMPOOL_FLAG_HUGE_PAGES_ALLOC,
                      &slab->pool->flags))
                msf_free_huge_pages(region->buf);
            else if (MSF_TEST_BITS(MSF_MEMPOOL_FLAG_NUMA_ALLOC,
                           &slab->pool->flags))
                msf_numa_free_ptr(region->buf);
            else if (MSF_TEST_BITS(MSF_MEMPOOL_FLAG_REGULAR_PAGES_ALLOC,
                       &slab->pool->flags))
                sfree(region->buf);

            sfree(region);
            return NULL;
        }
    }

    qblock = &dummy;
    pblock = block;
    for (i = 0; i < nr_blocks; i++) {
        list_add(&pblock->blocks_list_entry, &slab->blocks_list);

        pblock->parent_slab = slab;
        pblock->omr	= region->omr;
        pblock->buf	= (char *)(region->buf) + i * aligned_sz;
        pblock->refcnt_claim = 1; /* free - claimed be MP */
        qblock->next = pblock;
        qblock = pblock;
        pblock++;
    }

    /* first block given to allocator */
    if (alloc) {
        if (nr_blocks == 1)
            pblock = NULL;
        else
            pblock = block + 1;
        block->next = NULL;
        /* ref count 1, not claimed by MP */
        block->refcnt_claim = 2;
    } else {
        pblock = block;
    }
    /* Concatenate [pblock -- qblock] to free list
     * qblock points to the last allocate block
     */

    if (slab->pool->safe_mt) {
        do {
            qblock->next = slab->free_blocks_list;
        } while (!msf_atomic_cmp_set(
                &slab->free_blocks_list,
                qblock->next, pblock));
    } else  {
        qblock->next = slab->free_blocks_list;
        slab->free_blocks_list = pblock;
    }

    slab->curr_mb_nr += nr_blocks;

    list_add(&region->mem_region_entry, &slab->mem_regions_list);

    return block;
}

/*---------------------------------------------------------------------------*/
/* msf_mempool_destroy							     */
/*---------------------------------------------------------------------------*/
void msf_mempool_destroy(struct msf_mempool *p)
{
    u32 i;

    if (!p)
        return;

    for (i = 0; i < p->slabs_nr; i++)
        msf_mem_slab_free(&p->slab[i]);

    sfree(p->slab);
    sfree(p);
}

/*---------------------------------------------------------------------------*/
/* msf_mempool_dump							     */
/*---------------------------------------------------------------------------*/
void msf_mempool_dump(struct msf_mempool *p)
{
    u32 i;
    struct msf_mem_slab *s;

    if (!p)
        return;

    printf("------------------------------------------------\n");
    for (i = 0; i < p->slabs_nr; i++) {
        s = &p->slab[i];
        printf("pool:%p - slab[%u]: " \
             "size:%zd, used:%u, alloced:%d, max_alloc:%d\n",
            p, i, s->mb_size, s->used_mb_nr,
            s->curr_mb_nr, s->max_mb_nr);
    }
    printf("------------------------------------------------\n");
}

/*---------------------------------------------------------------------------*/
/* MSF_mempool_create     */
/*---------------------------------------------------------------------------*/
struct msf_mempool *msf_mempool_create(s32 nodeid, u32 flags)
{
    struct msf_mempool *p;

    if (MSF_TEST_BITS(MSF_MEMPOOL_FLAG_HUGE_PAGES_ALLOC, &flags)) {
        MSF_CLR_BITS(MSF_MEMPOOL_FLAG_REGULAR_PAGES_ALLOC, &flags);
        MSF_CLR_BITS(MSF_MEMPOOL_FLAG_NUMA_ALLOC, &flags);
        printf("mempool: using huge pages allocator\n");
    } else if (MSF_TEST_BITS(MSF_MEMPOOL_FLAG_NUMA_ALLOC, &flags)) {
        MSF_CLR_BITS(MSF_MEMPOOL_FLAG_REGULAR_PAGES_ALLOC, &flags);
        printf("mempool: using numa allocator\n");
    } else {
        MSF_SET_BITS(MSF_MEMPOOL_FLAG_REGULAR_PAGES_ALLOC, &flags);
        printf("mempool: using regular allocator\n");
    }

    if (MSF_TEST_BITS(MSF_MEMPOOL_FLAG_NUMA_ALLOC, &flags)) {
        s32 ret;

        if (nodeid == -1) {
            int cpu = msf_get_current_cpu();

            nodeid = msf_numa_node_of_cpu(cpu);
        }
        /* pin to node */
        ret = msf_numa_run_on_node(nodeid);
        if (ret)
            return NULL;
    }

    p = (struct msf_mempool *)calloc(1, sizeof(struct msf_mempool));
    if (!p)
        return NULL;

    p->nodeid = nodeid;
    p->flags = flags;
    p->slabs_nr = 0;
    p->safe_mt = 1;
    p->slab = NULL;

    return p;
}

/*---------------------------------------------------------------------------*/
/* msf_mempool_create_prv						     */
/*---------------------------------------------------------------------------*/
struct msf_mempool *msf_mempool_create_prv(s32 nodeid, u32 flags)
{
    struct msf_mempool  *p;
    size_t  i;
    s32 ret;

    if (g_mempool_config.slabs_nr < 1 ||
        g_mempool_config.slabs_nr > MSF_MAX_SLABS_NR) {
        errno = EINVAL;
        return NULL;
    }

    p = msf_mempool_create(nodeid, flags);
    if (!p)
        return  NULL;

    for (i = 0; i < g_mempool_config.slabs_nr; i++) {
        ret = msf_mempool_add_slab(
            p,
            g_mempool_config.slab_cfg[i].block_sz,
            g_mempool_config.slab_cfg[i].init_blocks_nr,
            g_mempool_config.slab_cfg[i].max_blocks_nr,
            g_mempool_config.slab_cfg[i].grow_blocks_nr,
            0); /*default alignment */
        if (ret != 0)
            goto cleanup;
    }

    return p;

cleanup:
    msf_mempool_destroy(p);
    return NULL;
}

/*---------------------------------------------------------------------------*/
/* size2index								     */
/*---------------------------------------------------------------------------*/
static inline s32 size2index(struct msf_mempool *p, size_t sz)
{
    u32 i;

    for (i = 0; i <= p->slabs_nr; i++)
        if (sz <= p->slab[i].mb_size)
            break;

    return (i == p->slabs_nr) ? -1 : (s32)i;
}

/*---------------------------------------------------------------------------*/
/* msf_mempool_alloc							     */
/*---------------------------------------------------------------------------*/
s32 msf_mempool_alloc(struct msf_mempool *p, size_t length,
              struct msf_reg_mem *reg_mem)
{
    s32 index;
    struct msf_mem_slab *slab;
    struct msf_mem_block *block;
    s32 ret = 0;

    index = size2index(p, length);
    printf("msf_mempool_alloc slab index:%d.\n", index);

retry:
    if (index == -1) {
        printf("msf_mempool_alloc slab index:%d.\n", index);
        errno = EINVAL;
        ret = -1;
        reg_mem->addr   = NULL;
        reg_mem->mr     = NULL;
        reg_mem->priv   = NULL;
        reg_mem->length = 0;
        goto cleanup;
    }
    slab = &p->slab[index];

    if (p->safe_mt)
        block = safe_new_block(slab);
    else
        block = non_safe_new_block(slab);
    if (!block) {
        if (p->safe_mt) {
            pthread_spin_init(&slab->lock, 0);
        /* we may been blocked on the spinlock while other
         * thread resized the pool
         */
            block = safe_new_block(slab);
        } else {
            block = non_safe_new_block(slab);
        }
        if (!block) {
            block = msf_mem_slab_resize(slab, 1);
            if (!block) {
                if (++index == (int)p->slabs_nr ||
                    MSF_TEST_BITS(
                    MSF_MEMPOOL_FLAG_USE_SMALLEST_SLAB,
                    &p->flags))
                    index  = -1;

                if (p->safe_mt)
                    pthread_spin_unlock(&slab->lock);
                ret = 0;
                printf("1resizing slab size:%zd\n", slab->mb_size);
                goto retry;
            }
            printf("2resizing slab size:%zd\n", slab->mb_size);
        }
        if (p->safe_mt)
            pthread_spin_unlock(&slab->lock);
    }

    reg_mem->addr   = block->buf;
    reg_mem->mr     = block->omr;
    reg_mem->priv   = block;
    reg_mem->length = length;

#ifdef DEBUG_MEMPOOL_MT
    msf_atomic_fetch_sub(&slab->used_mb_nr, 1);
    if (msf_atomic_fetch_sub(&block->refcnt, 1) != 0) {
        printf("pool alloc failed\n");
        abort(); /* core dump - double free */
    }
#else
    msf_atomic_fetch_add(&slab->used_mb_nr, 1);

#endif

cleanup:
#ifdef DEBUG_MEMPOOL_MT
    msf_mempool_dump(p);
#endif
    return ret;
}

/*---------------------------------------------------------------------------*/
/* msf_mempool_free							     */
/*---------------------------------------------------------------------------*/
void msf_mempool_free(struct msf_reg_mem *reg_mem)
{
    struct msf_mem_block	*block;

    if (!reg_mem || !reg_mem->priv)
        return;

    block = (struct msf_mem_block *)reg_mem->priv;

#ifdef DEBUG_MEMPOOL_MT
    if (msf_atomic_fetch_sub(&block->refcnt, 1) != 1) {
        printf("pool: release failed");
        abort(); /* core dump - double free */
    }
    msf_atomic_fetch_sub(&block->parent_slab->used_mb_nr, 1);
#else
    block->parent_slab->used_mb_nr--;
#endif

   msf_atomic_fetch_sub(&block->parent_slab->used_mb_nr, 1);

    if (block->parent_slab->pool->safe_mt)
        safe_release(block->parent_slab, block);
    else
        non_safe_release(block->parent_slab, block);
}

/*---------------------------------------------------------------------------*/
/* msf_mempool_add_slab							     */
/*------------------------------------------------------------------*/
s32 msf_mempool_add_slab(struct msf_mempool *p,
        size_t size, size_t min, size_t max,
        size_t alloc_quantum_nr, s32 alignment)
{
    struct msf_mem_slab	*new_slab;
    struct msf_mem_block	*block;
    u32 ix, slab_ix, slab_shift = 0;
    s32 align = alignment;

    slab_ix = p->slabs_nr;
    if (p->slabs_nr) {
        for (ix = 0; ix < p->slabs_nr; ++ix) {
            if (p->slab[ix].mb_size == size) {
                errno = EEXIST;
                return -1;
            }
            if (p->slab[ix].mb_size > size) {
                slab_ix = ix;
                break;
            }
        }
    }
    if (!alignment) {
        //align = g_options.xfer_buf_align;
    } else if (!is_power_of_2(alignment) ||
           !(alignment % sizeof(void *) == 0)) {
        printf("invalid alignment %d\n", alignment);
        errno = EINVAL;
        return -1;
    }

    /* expand */
    new_slab = (struct msf_mem_slab *)calloc(p->slabs_nr + 2,
                     sizeof(struct msf_mem_slab));
    if (!new_slab) {
        errno = ENOMEM;
        return -1;
    }

    /* fill/shift slabs */
    for (ix = 0; ix < p->slabs_nr + 1; ++ix) {
        if (ix == slab_ix) {
            /* new slab */
            new_slab[ix].pool = p;
            new_slab[ix].mb_size = size;
            new_slab[ix].init_mb_nr = min;
            new_slab[ix].max_mb_nr = max;
            new_slab[ix].alloc_quantum_nr = alloc_quantum_nr;
            new_slab[ix].align = align;

            pthread_spin_init(&new_slab[ix].lock, 0);
            INIT_LIST_HEAD(&new_slab[ix].mem_regions_list);
            INIT_LIST_HEAD(&new_slab[ix].blocks_list);
            new_slab[ix].free_blocks_list = NULL;
            if (new_slab[ix].init_mb_nr) {
                (void)msf_mem_slab_resize(
                &new_slab[ix], 0);
            }
            /* src adjust */
            slab_shift = 1;
            continue;
        }
        /* shift it */
        new_slab[ix] = p->slab[ix - slab_shift];
        INIT_LIST_HEAD(&new_slab[ix].mem_regions_list);
        list_splice_init(&p->slab[ix - slab_shift].mem_regions_list,
                 &new_slab[ix].mem_regions_list);
        INIT_LIST_HEAD(&new_slab[ix].blocks_list);
        list_splice_init(&p->slab[ix - slab_shift].blocks_list,
                 &new_slab[ix].blocks_list);
        list_for_each_entry(block, &new_slab[ix].blocks_list,
                   blocks_list_entry) {
            block->parent_slab = &new_slab[ix];
        }
    }

    /* sentinel */
    new_slab[p->slabs_nr + 1].mb_size = SIZE_MAX;

    /* swap slabs */
    sfree(p->slab);
    p->slab = new_slab;

    /* adjust length */
    (p->slabs_nr)++;

    return 0;
}


s32 msf_mem_register(void *addr, size_t length, struct msf_reg_mem *reg_mem)
{
    static struct msf_mr dummy_mr;

    if (!addr || !reg_mem) {
        errno = EINVAL;
        return -1;
    }

    reg_mem->addr = addr;
    reg_mem->length = length;
    reg_mem->mr = &dummy_mr;

    return 0;
}

s32 msf_mem_dereg(struct msf_reg_mem *reg_mem)
{
    reg_mem->mr = NULL;
    return 0;
}

s32 msf_mem_alloc(size_t length, struct msf_reg_mem *reg_mem) {
    size_t  real_size;
    s32 alloced = 0;
    long page_size = g_os->pagesize;

    real_size = MSF_ALIGN(length, page_size);
    if (msf_memalign(&reg_mem->addr, page_size, real_size) != 0) {
        printf("xio_memalign failed. sz:%zd\n", real_size);
        reg_mem->addr = NULL;
        goto cleanup;
    }
    
    /*memset(reg_mem->addr, 0, real_size);*/
    alloced = 1;

    msf_mem_register(reg_mem->addr, length, reg_mem);
    if (!reg_mem->mr) {
        printf("xio_reg_mr failed. addr:%p, length:%zd.",
             reg_mem->addr, length);

    goto cleanup1;
    }
    reg_mem->length = length;

    return 0;

cleanup1:
    if (alloced)
        sfree(reg_mem->addr);
cleanup:
    return -1;
}

s32 msf_mem_free(struct msf_reg_mem *reg_mem) {
    int retval = 0;

    sfree(reg_mem->addr);

    retval = msf_mem_dereg(reg_mem);

    return retval;
}




