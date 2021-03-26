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
#ifndef BASE_MEM_MEMPOOL_H_
#define BASE_MEM_MEMPOOL_H_

#include <algorithm>
#include <array>
#include <atomic>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "atomic.h"
#include "mem.h"
#include "noncopyable.h"

using namespace MSF;

namespace MSF {

// 参考官方判断方法：
// http://www.opensource.apple.com/source/cctools/cctools-870/include/mach/arm/thread_status.h
// https://blog.csdn.net/liuqun69/article/details/103041250
#if (__i386__ || __i386 || __arm__)
#define MP_CACHE_LINE_ALIGN 4
#elif(__amd64__ || __amd64 || __x86_64 || __x86_64__ || __arm64__ || \
      __aarch64__)
#define MP_CACHE_LINE_ALIGN 8
#else
#define MP_CACHE_LINE_ALIGN 4
#endif

enum MemBlkIdx {
  MEMBLK_32B,
  MEMBLK_64B,
  MEMBLK_128B,
  MEMBLK_256B,
  MEMBLK_512B,
  MEMBLK_1K,
  MEMBLK_2K,
  MEMBLK_4K,
  MEMBLK_8K,
  MEMBLK_1M, /* huge pages */
  MEMBLK_2M, /* huge pages */
  MEMBLK_1G, /* huge pages */
  MEMBLK_MAX = 12,
  MEMBLK_ZERO = UINT32_MAX
};

enum MemFlag {
  MP_FLAG_NONE = 0x0000,
  MP_FLAG_REGULAR_PAGES_ALLOC = 0x0001,
  MP_FLAG_HUGE_PAGES_ALLOC = 0x0002,
  MP_FLAG_NUMA_ALLOC = 0x0004,
  MP_FLAG_USE_SMALLEST_SLAB = 0x0016
};

struct MemBlk {
  MemBlkIdx blk_idx_; /* cannot been changed */
  /* https://blog.csdn.net/ubuntu64fan/article/details/17629509 */
  /* ref_cnt aim to solve wildpointer and object, pointer retain */
  uint32_t ref_cnt_;
  uint32_t used_size_;
  uint32_t magic_;
  void *buffer_;

  MemBlk() {}
  MemBlkIdx blk_id() const { return blk_idx_; }
  void set_blk_id(MemBlkIdx id) { blk_idx_ = id; }
  uint32_t ref_cnt() const { return ref_cnt_; }
  void set_ref_cnt(uint32_t cnt) { ref_cnt_ = cnt; }
  uint32_t used_size() const { return used_size_; }
  void set_used_size(uint32_t size) { used_size_ = size; }
  uint32_t magic() const { return magic_; }
  void set_magic(uint32_t magic) { magic_ = magic; }
  void *buffer() { return buffer_; }
  void set_buffer(void *buffer) { buffer_ = buffer; }
};

/* Per slab config */
struct MemSlab {
  MemBlkIdx blk_idx_;
  uint32_t min_blk_nr_;
  uint32_t max_blk_nr_;
  uint32_t grow_blk_nr_;

  uint32_t padding_;
  uint32_t used_blk_nr_;
  std::list<MemBlk> free_blk_list_;
  ;
  std::list<MemBlk> used_blk_list_;
  uint32_t flags_;

  MemSlab(enum MemBlkIdx blkIdx, uint32_t minBlkNR, uint32_t maxBlkNR,
          uint32_t growBlkNR) {
    blk_idx_ = blkIdx;
    min_blk_nr_ = minBlkNR;
    max_blk_nr_ = maxBlkNR;
    grow_blk_nr_ = growBlkNR;
  }

  void set_blk_id(MemBlkIdx id) { blk_idx_ = id; }
  MemBlkIdx blk_id() const { return blk_idx_; }
  uint32_t min_blk_nr() const { return min_blk_nr_; }
  uint32_t max_blk_nr() const { return max_blk_nr_; }
  uint32_t grow_blk_nr() const { return grow_blk_nr_; }
  uint32_t used_blk_nr() const { return used_blk_nr_; }

  bool FreeBlkEmpty() const { return free_blk_list_.empty(); }
  inline void AddFreeBlk(const MemBlk &blk) {
    free_blk_list_.push_back(std::move(blk));
  }
  void *AllocBlkBuffer() {
    if (free_blk_list_.empty()) {
      return nullptr;
    }
    auto &blk = free_blk_list_.front();
    free_blk_list_.pop_front();
    return blk.buffer();
  }
};

class MemPool : noncopyable {
 public:
  explicit MemPool()
      : numa_id_(-1),
        safe_mt_(true),
        blk_algin_(MP_CACHE_LINE_ALIGN),
        flags_(0),
        mutex_() {
    AddDefMemSlab();
  }
  explicit MemPool(const int numaId, const uint32_t blkAlign,
                   bool safeMt = true)
      : numa_id_(numaId),
        safe_mt_(safeMt),
        blk_algin_(blkAlign),
        flags_(0),
        mutex_() {
    AddDefMemSlab();
  }
  ~MemPool();

  void set_numa_id(const int numaId) { numa_id_ = numaId; }
  const int numa_id() const { return numa_id_; }
  void set_blk_align(const uint32_t blkAlign) { blk_algin_ = blkAlign; }
  const uint32_t blk_algin() const { return blk_algin_; }

  bool Init();
  void *Alloc(const uint32_t size);
  void Free(const void *ptr);
  void ReduceBlk();

 private:
  int numa_id_;
  bool safe_mt_; /* Mutithread support */

  uint32_t blk_algin_;
  uint32_t max_slab_;
  std::vector<MemSlab> slabs_;
  uint32_t flags_;

  /* Use memblk buffer addr to find slab,
   * then put memblk to freelist */
  std::map<uint64_t, uint64_t> slab_addr_map_;
  std::mutex mutex_;

  bool CreateSlab();
  bool DestroySlab();

  inline MemBlkIdx GetMemBlkIdx(const uint32_t size);
  void AddMemSlab(MemBlkIdx blkIdx, uint32_t minBlkNR, uint32_t maxBlkNR,
                  uint32_t growBlkNR);
  void AddDefMemSlab();
  inline void MapBufferAddr(const MemSlab &slab, const void *buffer);
};

#if 0
template 
< typename ElemT,
  uint32_t ChunkSize = 65534,
  typename Allocator = std::allocator<ElemT>
>
class ObjMempool : public Mempool {
    public:
        ObjMempool(): ObjMempool(Allocator())
        {
        }

        ObjMempool(Allocator & alloc)
        {

        }

        ~ObjMempool()
        {
            // std::allocator_traits<Allocator>::deallocate(allocator, arena, arena_size);
            // assert(count == ChunkSize);
        }
        // Allocates an element in the pool, using arguments passed into the function
        // to call a corresponding constructor.
        // Never returns null.
        // Throws std::bad_alloc if allocation is failed.
        // If constructor throws, the allocated element is returned to the pool
        // and the exception is rethrown
        template<typename... Args>
        [[nodiscard]] ElemT* alloc(Args&& ...args)
        {
            auto elem = allocate();
            if constexpr(std::is_nothrow_constructible_v<ElemT, Args...>)
            {
                return ::new(elem) ElemT(std::forward<Args>(args)...);
            }
            else try {
                return ::new(elem) ElemT(std::forward<Args>(args)...);
            } catch(...) {
                free(elem);
                throw;
            }
        }

         // Calls an element's destructor and returns a block of memory into the pool.
        // Never throws an excepton.
        void free(ElemT* elem) noexcept
        {
            if (elem == nullptr) return;

            elem->~ElemT();
            deallocate(elem);
        }

        // bool contains(ElemT* elem) const noexcept { return elem >= arena && elem < arena + ChunkSize; }
        // bool getIndex(ElemT* elem) const noexcept { return std::distance(arena, elem); }

        //---------------------------------------------------------------------
        // Gets an uninitialized block of memory from the pool.
        // No ElemT constructors are called, so you must use a placement new() 
        // on the returned pointr and choose which constructor to call.
        // Throws std::bad_alloc if allocation is failed.
        [[nodiscard]] ElemT* allocate()
        {

        }

        //https://github.com/a-bronx/MemPool/blob/master/src/Mempool.h
        //https://github.com/IthacaDream/MemPool/blob/master/memory_pool.h
        //https://github.com/lolonet/MemPool/blob/master/MemManager.cpp
    private:
        static_assert(std::is_same_v<ElemT, Allocator::value_type>, "Incompatible allocator type");
        static_assert(std::is_nothrow_destructible_v<ElemT>, "Element destructor must be noexcept");

        Allocator allocator;
    
};
#endif

}  // namespace MSF
#endif