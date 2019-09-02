
#ifndef __MSF_SHMEM_H__
#define __MSF_SHMEM_H__

#include <msf_utils.h>

typedef struct {
/*
共享内存的其实地址开始处数据:ngx_slab_pool_t + 9 * sizeof(ngx_slab_page_t)(slots_m[]) + pages * sizeof(ngx_slab_page_t)(pages_m[]) +pages*ngx_pagesize(这是实际的数据部分，
每个ngx_pagesize都由前面的一个ngx_slab_page_t进行管理，并且每个ngx_pagesize最前端第一个obj存放的是一个或者多个int类型bitmap，用于管理每块分配出去的内存)
*/ //见ngx_init_zone_pool，共享内存的起始地址开始的sizeof(ngx_slab_pool_t)字节是用来存储管理共享内存的slab poll的
    u8      *addr; //共享内存起始地址  
    size_t  size; //共享内存空间大小
    s8      *name; //这块共享内存的名称
    u32     exists;   /* unsigned  exists:1;  */ //表示共享内存是否已经分配过的标志位，为1时表示已经存在
} ngx_shm_t;


s32 ngx_shm_alloc(ngx_shm_t *shm);
void ngx_shm_free(ngx_shm_t *shm);

