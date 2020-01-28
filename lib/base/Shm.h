
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
#ifndef __MSF_SHM_H__
#define __MSF_SHM_H__

#include <iostream>

namespace MSF {
namespace MEM {


/*
共享内存的其实地址开始处数据:ngx_slab_pool_t + 9 * sizeof(ngx_slab_page_t)(slots_m[]) + pages * sizeof(ngx_slab_page_t)(pages_m[]) +pages*ngx_pagesize(这是实际的数据部分，
每个ngx_pagesize都由前面的一个ngx_slab_page_t进行管理，并且每个ngx_pagesize最前端第一个obj存放的是一个或者多个int类型bitmap，用于管理每块分配出去的内存)
*/ 
//见ngx_init_zone_pool共享内存的起始地址开始的sizeof(ngx_slab_pool_t)字节是用来存储管理共享内存的slabpoll的

class MSF_SHM {
    public:
        MSF_SHM() = default;
        explicit MSF_SHM(const std::string &name, uint64_t size);
        virtual ~MSF_SHM()
        {
            ShmFree();
        }

        enum ShmStrategy {
            ALLOC_BY_MAPPING_FILE,
            ALLOC_BY_UNMAPPING_FILE,
            ALLOC_BY_MAPPING_DEVZERO,
            ALLOC_BY_SHMGET,
        };


        int ShmAlloc();
        void ShmFree();

    private:
        std::string mapfile;
        std::string name;
        uint64_t    size;
        uint8_t     *addr;
        enum ShmStrategy strategy;
        int ShmAllocByMapFile(const std::string & filename);
};

} /**************************** end namespace BASE ****************************/
} /**************************** end namespace MSF  ****************************/
#endif
