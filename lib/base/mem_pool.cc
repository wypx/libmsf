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
#include "mem_pool.h"

#include <base/logging.h>

#include "affinity.h"
#include "os.h"
#include "utils.h"

using namespace MSF;

namespace MSF {
/*
 *  Determine whether some value is a power of two, where zero is
 * *not* considered a power of two.
 */
static inline MSF_CONST_CALL bool IsPowerOf2(uint32_t n) {
  return (n != 0 && ((n & (n - 1)) == 0));
}

static struct MemSlab kDefaultMemSlab[] = {
    MemSlab(MEMBLK_32B, 4, 1024, 8),  MemSlab(MEMBLK_64B, 4, 1024, 8),
    MemSlab(MEMBLK_128B, 4, 1024, 8), MemSlab(MEMBLK_256B, 4, 1024, 8),
    MemSlab(MEMBLK_512B, 4, 1024, 8), MemSlab(MEMBLK_1K, 4, 1024, 8),
    MemSlab(MEMBLK_2K, 4, 1024, 8),   MemSlab(MEMBLK_4K, 4, 1024, 8),
    MemSlab(MEMBLK_8K, 4, 1024, 8),   MemSlab(MEMBLK_1M, 0, 1024, 8),
    MemSlab(MEMBLK_2M, 0, 1024, 8),   MemSlab(MEMBLK_1G, 0, 1, 0),
};

/* every block size in cmd mem pool */
static const uint32_t kMemBlkSizeArray[MEMBLK_MAX] = {32,
                                                      64,
                                                      128,
                                                      256,
                                                      512,
                                                      1024,
                                                      2048,
                                                      4096,
                                                      8096,
                                                      1024 * 1024 * 1,
                                                      1024 * 1024 * 2,
                                                      1024 * 1024 * 1024};

enum MemBlkIdx MemPool::GetMemBlkIdx(const uint32_t size) {
  if (unlikely(size == 0)) {
    LOG(ERROR) << "no fitable memblk for size: " << size;
    return MEMBLK_ZERO;
  }
  uint32_t alignSize = MSF_ROUNDUP(size, blk_algin_);
  if (unlikely(alignSize > kMemBlkSizeArray[MEMBLK_1G])) {
    LOG(ERROR) << "no fitable memblk for size: " << alignSize;
    return MEMBLK_MAX;
  }
  // https://blog.csdn.net/qq_40160605/article/details/80150252
  // https://www.cnblogs.com/wkfvawl/p/9475939.html
  // https://www.jianshu.com/p/cb0d5488bb6a
  uint32_t idx = std::upper_bound(kMemBlkSizeArray,
                                  kMemBlkSizeArray + MEMBLK_MAX, alignSize) -
                 kMemBlkSizeArray;

  LOG(INFO) << "mempool blk idx: " << idx << " size: " << kMemBlkSizeArray[idx];
  return (MemBlkIdx)idx;
}

MemPool::~MemPool() {}

void MemPool::MapBufferAddr(const MemSlab &slab, const void *buffer) {
  // https://blog.csdn.net/fengbingchun/article/details/70406274
  slab_addr_map_.insert(std::make_pair<uint64_t, uint64_t>(
      (uint64_t)buffer, (uint64_t)std::addressof(slab)));
}

bool MemPool::CreateSlab() {
  std::for_each(slabs_.begin(), slabs_.end(), [this](auto &slab) {
    // std::lock_guard<std::mutex> lock(itor.mutex_);
    /* region data */
    uint32_t alignSize = kMemBlkSizeArray[slab.blk_id()];
    for (uint32_t i = 0; i < slab.min_blk_nr(); ++i) {
      MemBlk blk;  // = (struct MemBlk*)malloc(sizeof(struct MemBlk));
      /* allocate the buffers and register them */
      if (MSF_TEST_BITS(MP_FLAG_HUGE_PAGES_ALLOC, &flags_))
        blk.set_buffer(BaseAllocHugePages(alignSize));
      else if (MSF_TEST_BITS(MP_FLAG_NUMA_ALLOC, &flags_))
        blk.set_buffer(BaseNumaAllocPages(alignSize, numa_id_));
      else if (MSF_TEST_BITS(MP_FLAG_REGULAR_PAGES_ALLOC, &flags_)) {
        // blk._buffer = BasicAlignedAlloc(blk_algin_, alignSize);
        blk.set_buffer(malloc(alignSize));
      }
      if (blk.buffer() == nullptr) {
        LOG(INFO) << "alloc slab buffer faild, errno:" << errno;
        return false;

        // free_list_ = (void **)malloc(chunk_cnt_ * sizeof(void *));
        // int flags = MAP_PRIVATE | MAP_ANONYMOUS;
        // if (enable_huge_page_) {
        //     flags |= MAP_HUGETLB;
        // }
        // void *mem_start_ = mmap(NULL, chunk_size_ * chunk_cnt_, PROT_READ |
        // PROT_WRITE,
        //                     flags, -1, 0);
        // if (mem_start_ == MAP_FAILED) {
        // }
      }
      // LOG(DEBUG << "BlkIdx: " << slab._blkIdx
      //         << " BlkSize: " << alignSize
      //         << " BlkBuff: " << blk._buffer
      //         << " SlabAddr: " << &slab;
      blk.set_blk_id(slab.blk_id());
      slab.AddFreeBlk(blk);
      // https://blog.csdn.net/fengbingchun/article/details/70406274
      MapBufferAddr(slab, blk.buffer());
      slab_addr_map_.insert(std::make_pair<uint64_t, uint64_t>(
          (uint64_t)blk.buffer(), (uint64_t)std::addressof(slab)));
    }
    return true;
  });
  return true;
}

bool MemPool::DestroySlab() {
  // if (free_list_) {
  //     free(free_list_);
  // }
  // if (mem_start_) {
  //     munmap(mem_start_, chunk_size_ * chunk_cnt_);
  // }
  return true;
}

bool MemPool::Init() {
  if (blk_algin_ == 0) {
    blk_algin_ = MP_CACHE_LINE_ALIGN;
  } else if (!IsPowerOf2(blk_algin_) || !(blk_algin_ % sizeof(void *) == 0)) {
    LOG(ERROR) << "invalid alignment: " << blk_algin_;
    return false;
  }

  LOG(INFO) << "flag is:" << flags_;
  if (MSF_TEST_BITS(MP_FLAG_HUGE_PAGES_ALLOC, &flags_)) {
    MSF_CLR_BITS(MP_FLAG_REGULAR_PAGES_ALLOC, &flags_);
    MSF_CLR_BITS(MP_FLAG_NUMA_ALLOC, &flags_);
    LOG(INFO) << "using huge pages allocator.";
  } else if (MSF_TEST_BITS(MP_FLAG_NUMA_ALLOC, &flags_)) {
    MSF_CLR_BITS(MP_FLAG_REGULAR_PAGES_ALLOC, &flags_);
    LOG(INFO) << "using numa allocator.";
  } else {
    MSF_SET_BITS(MP_FLAG_REGULAR_PAGES_ALLOC, &flags_);
    LOG(INFO) << "using regular allocator.";
  }

  // if (MSF_TEST_BITS(MP_FLAG_NUMA_ALLOC, &flags_)) {
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
  return CreateSlab();
}

void *MemPool::Malloc(size_t size) {
  uint32_t idx = GetMemBlkIdx(size);
  if (unlikely(idx == MEMBLK_MAX || idx == MEMBLK_ZERO)) {
    LOG(INFO) << "alloc memblk faild, size: " << size;
    return nullptr;

    // 根据 blockSize 计算需要预分配的块数，采用指数退让的做法，blockSize
    // 越大，那么预分配的块数越少 const uint32_t i = std::max(1, (int)size >>
    // 10); const uint32_t count = std::max(1, (int)(MAX_BLOCKLIST_COUNT/i) );
    // return count;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  auto &slab = slabs_[idx];
  LOG(INFO) << "slab addr: " << &slab << " size: " << size;
  return slab.AllocBlkBuffer();
}

void MemPool::Free(const void *ptr) {
  if (unlikely(ptr == nullptr)) {
    LOG(FATAL) << "buffer invalid.";
    return;
  }

  auto itor = slab_addr_map_.find((uint64_t)ptr);
  if (unlikely(itor == slab_addr_map_.end())) {
    LOG(FATAL) << "buffer invalid, not find key.";
    return;
  }
  struct MemSlab *slab = (struct MemSlab *)(itor->second);
  void *p = const_cast<void *>(ptr);
  struct MemBlk *blk = container_of(&p, struct MemBlk, buffer_);
  LOG(INFO) << "slab addr: " << slab << " memblk addr: " << blk
            << " buff addr: " << ptr;
  slab->AddFreeBlk(*blk);

  // 计算当前 blockSize 所在的链表，应该释放掉多少块内存给系统，做内存收缩
  // 采用的收缩规则是 空闲块大于初始分配的2倍以上，则做收缩
}

void MemPool::AddMemSlab(enum MemBlkIdx blkIdx, uint32_t minBlkNR,
                         uint32_t maxBlkNR, uint32_t growBlkNR) {
  if (unlikely(slabs_.size() > max_slab_)) {
    LOG(ERROR) << "not support any more slab config: " << max_slab_;
    return;
  }
  struct MemSlab slab(blkIdx, minBlkNR, maxBlkNR, growBlkNR);
  slabs_.push_back(std::move(slab));
}

void MemPool::AddDefMemSlab() {
  for (uint32_t i = 0; i < MSF_ARRAY_SIZE(kDefaultMemSlab); ++i) {
    slabs_.push_back(std::move(kDefaultMemSlab[i]));
  }
}

}  // namespace MSF
