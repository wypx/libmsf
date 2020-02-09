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
#include <base/MemPool.h>
#include <base/Utils.h>
#include <base/Os.h>
#include <base/Affinity.h>

using namespace MSF::BASE;

namespace MSF {
namespace BASE {

/*
 *  Determine whether some value is a power of two, where zero is
 * *not* considered a power of two.
 */
static inline MSF_CONST_CALL bool isPowerOf2(uint32_t n)
{
    return (n != 0 && ((n & (n - 1)) == 0));
}

static struct MemSlab _DefaultMemSlab_[] = {
    MemSlab(MEMBLK_64B, 4, 1024, 8),
    MemSlab(MEMBLK_128B, 4, 1024, 8),
    MemSlab(MEMBLK_256B, 4, 1024, 8),
    MemSlab(MEMBLK_512B, 4, 1024, 8),
    MemSlab(MEMBLK_1K, 4, 1024, 8),
    MemSlab(MEMBLK_2K, 4, 1024, 8),
    MemSlab(MEMBLK_4K, 4, 1024, 8),
    MemSlab(MEMBLK_8K, 4, 1024, 8),
    MemSlab(MEMBLK_1M, 0, 1024, 8),
    MemSlab(MEMBLK_2M, 0, 1024, 8),
    MemSlab(MEMBLK_1G, 0, 1, 0),
};

/* every block size in cmd mem pool */
static const uint32_t _MemBlkSizeArray_[MEMBLK_MAX] = 
    { 64, 128, 256, 512, 1024, 2048, 4096, 8096,
      1024 * 1024 * 1, 1024 * 1024 * 2, 1024 * 1024 * 1024 };

enum MemBlkIdx MemPool::getMemBlkIdx(const uint32_t size)
{
    if (unlikely(size == 0)) {
        MSF_ERROR << "No fitable memblk for size: " << size;
        return MEMBLK_ZERO;
    }
    uint32_t alignSize = MSF_ROUNDUP(size, _blkAlign);
    if (unlikely(alignSize > _MemBlkSizeArray_[MEMBLK_1G])) {
        MSF_ERROR << "No fitable memblk for size: " << alignSize;
        return MEMBLK_MAX;
    }
    //https://blog.csdn.net/qq_40160605/article/details/80150252
    //https://www.cnblogs.com/wkfvawl/p/9475939.html
    //https://www.jianshu.com/p/cb0d5488bb6a
    uint32_t idx = std::upper_bound(_MemBlkSizeArray_,
        _MemBlkSizeArray_ + MEMBLK_MAX, alignSize) - _MemBlkSizeArray_;
 
    MSF_INFO << "Mempool blk idx: " << idx
            << " size: " << _MemBlkSizeArray_[idx];
    return (MemBlkIdx)idx;
}

MemPool::~MemPool()
{

}

bool MemPool::createSlab()
{
    std::for_each(_slabs.begin(), _slabs.end(), [this](auto & slab) {
        // std::lock_guard<std::mutex> lock(itor._mutex);
        /* region data */
        uint32_t alignSize = _MemBlkSizeArray_[slab._blkIdx];
        for (uint32_t i = 0; i < slab._minBlkNR; ++i) {
            struct MemBlk blk; // = (struct MemBlk*)malloc(sizeof(struct MemBlk));
            /* allocate the buffers and register them */
            if (MSF_TEST_BITS(MP_FLAG_HUGE_PAGES_ALLOC, &_flags))
                blk._buffer = BaseAllocHugePages(alignSize);
            else if (MSF_TEST_BITS(MP_FLAG_NUMA_ALLOC, &_flags))
                blk._buffer = BaseNumaAllocPages(alignSize, _numaId);
            else if (MSF_TEST_BITS(MP_FLAG_REGULAR_PAGES_ALLOC, &_flags)) {
                // blk._buffer = BasicAlignedAlloc(_blkAlign, alignSize);
                blk._buffer = malloc(alignSize);
            }
            if (blk._buffer == nullptr) {
                MSF_INFO << "Alloc slab buffer faild, errno:" << errno;
                return false;

                // free_list_ = (void **)malloc(chunk_cnt_ * sizeof(void *));
                // int flags = MAP_PRIVATE | MAP_ANONYMOUS;
                // if (enable_huge_page_) {
                //     flags |= MAP_HUGETLB;
                // }
                // void *mem_start_ = mmap(NULL, chunk_size_ * chunk_cnt_, PROT_READ | PROT_WRITE,
                //                     flags, -1, 0);
                // if (mem_start_ == MAP_FAILED) {
                // }
            }
            MSF_DEBUG << "BlkIdx: " << slab._blkIdx
                    << " BlkSize: " << alignSize
                    << " BlkBuff: " << blk._buffer
                    << " SlabAddr: " << &slab;
            blk._blkIdx = slab._blkIdx;
            slab._freeBlkList.push_back(std::move(blk));
            //https://blog.csdn.net/fengbingchun/article/details/70406274
            _slabBuffMap.insert(
                std::make_pair<uint64_t, uint64_t>(
                    (uint64_t)blk._buffer,
                    (uint64_t)std::addressof(slab)));
        }
        return true;
    });
    return true;
}

bool MemPool::destroySlab()
{
    // if (free_list_) {
    //     free(free_list_);
    // }
    // if (mem_start_) {
    //     munmap(mem_start_, chunk_size_ * chunk_cnt_);
    // }
    return true;
}

bool MemPool::init()
{
    if (_blkAlign == 0) {
        _blkAlign = MP_CACHE_LINE_ALIGN;
    } else if (!isPowerOf2(_blkAlign) 
            || !(_blkAlign % sizeof(void *) == 0)) {
        MSF_ERROR << "Invalid alignment: " << _blkAlign;
        return false;
    }

    MSF_INFO << "Flag is:" << _flags;
    if (MSF_TEST_BITS(MP_FLAG_HUGE_PAGES_ALLOC, &_flags)) {
        MSF_CLR_BITS(MP_FLAG_REGULAR_PAGES_ALLOC, &_flags);
        MSF_CLR_BITS(MP_FLAG_NUMA_ALLOC, &_flags);
        MSF_INFO << "Using huge pages allocator.";
    } else if (MSF_TEST_BITS(MP_FLAG_NUMA_ALLOC, &_flags)) {
        MSF_CLR_BITS(MP_FLAG_REGULAR_PAGES_ALLOC, &_flags);
        MSF_INFO << "Using numa allocator.";
    } else {
        MSF_SET_BITS(MP_FLAG_REGULAR_PAGES_ALLOC, &_flags);
        MSF_INFO<< "Using regular allocator.";
    }

    // if (MSF_TEST_BITS(MP_FLAG_NUMA_ALLOC, &_flags)) {
    //     int ret;
    //     int cpuId;
    //     if (_numaId == -1) {
    //         cpuId = msf_get_current_cpu();
    //         _numaId = BasicNumaNodeOfCpu(cpuId);
    //     }
    //     /* pin to node */
    //     ret = BasicNumaRunOnNode(_numaId);
    //     if (ret) return -1;
    // }
    return createSlab();
}

void * MemPool::alloc(const uint32_t size)
{
    uint32_t idx = getMemBlkIdx(size);
    if (unlikely(idx == MEMBLK_MAX || idx == MEMBLK_ZERO)) {
        MSF_INFO << "Alloc memblk faild, size: " << size;
        return nullptr;

        // 根据 blockSize 计算需要预分配的块数，采用指数退让的做法，blockSize 越大，那么预分配的块数越少 
        // const uint32_t i = std::max(1, (int)size >> 10);
        // const uint32_t count = std::max(1, (int)(MAX_BLOCKLIST_COUNT/i) );
        // return count;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    auto & slab = _slabs[idx];
    MSF_INFO << "slab addr: " << &slab << " size: " << size;
    if (slab._freeBlkList.empty()) {
        MSF_INFO << "Alloc memblk faild, freeBlkList empty.";;
        return nullptr;
    }
    auto & blk = slab._freeBlkList.front();
    slab._freeBlkList.pop_front();
    return blk._buffer;
}

void MemPool::free(const void *ptr)
{
    if (unlikely(ptr == nullptr)) {
        MSF_FATAL << "Buffer invalid.";
        return;
    }

    auto itor = _slabBuffMap.find((uint64_t)ptr);
    if (unlikely(itor == _slabBuffMap.end())) {
        MSF_FATAL << "Buffer invalid, not find key.";
        return;
    }
    struct MemSlab* slab = (struct MemSlab*)(itor->second);
    void *p = const_cast<void*>(ptr);
    struct MemBlk *blk = container_of(&p, struct MemBlk, _buffer);
    MSF_INFO << "Slab addr: " << slab
            << " Memblk addr: " << blk
            << " Buff addr: " << ptr;
    slab->_freeBlkList.push_back(std::move(*blk));

    // 计算当前 blockSize 所在的链表，应该释放掉多少块内存给系统，做内存收缩
    // 采用的收缩规则是 空闲块大于初始分配的2倍以上，则做收缩
}

void MemPool::addMemSlab(enum MemBlkIdx blkIdx, uint32_t minBlkNR,
                uint32_t maxBlkNR, uint32_t growBlkNR)
{
    if (unlikely(_slabs.size() > _maxSlabs)) {
        MSF_ERROR << "Not support any more slab config: " << _maxSlabs;
        return;
    }
    struct MemSlab slab(blkIdx, minBlkNR, maxBlkNR, growBlkNR);
    _slabs.push_back(std::move(slab));
}

void MemPool::addDefMemSlab()
{
    for (uint32_t i = 0; i < MSF_ARRAY_SIZE(_DefaultMemSlab_); ++i) {
        _slabs.push_back(std::move(_DefaultMemSlab_[i]));
    }
}

} /**************************** end namespace BASE ****************************/
} /**************************** end namespace MSF  ****************************/
