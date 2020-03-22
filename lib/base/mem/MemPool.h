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
#ifndef BASE_MEM_MEMPOOL_H
#define BASE_MEM_MEMPOOL_H

#include <base/Atomic.h>
#include <base/mem/Mem.h>
#include <base/Noncopyable.h>

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

using namespace MSF::BASE;

namespace MSF {
namespace BASE {

#if (__i386__ || __i386)
#define MP_CACHE_LINE_ALIGN 4
#elif (__amd64__ || __amd64)
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
  enum MemBlkIdx _blkIdx; /* cannot been changed */
  /* https://blog.csdn.net/ubuntu64fan/article/details/17629509 */
  /* ref_cnt aim to solve wildpointer and object, pointer retain */
  uint32_t _refCnt;
  uint32_t _usedSize;
  uint32_t _magic;
  void* _buffer;

  MemBlk() {}
};

/* Per slab config */
struct MemSlab {
  enum MemBlkIdx _blkIdx;
  uint32_t _minBlkNR;
  uint32_t _maxBlkNR;
  uint32_t _growBlkNR;

  uint32_t _padding;
  uint32_t _usedBlkNR;
  std::list<struct MemBlk> _freeBlkList;
  std::list<struct MemBlk> _usedBlkList;
  uint32_t _flags;

  MemSlab(enum MemBlkIdx blkIdx, uint32_t minBlkNR, uint32_t maxBlkNR,
          uint32_t growBlkNR) {
    _blkIdx = blkIdx;
    _minBlkNR = minBlkNR;
    _maxBlkNR = maxBlkNR;
    _growBlkNR = growBlkNR;
  }
};

class MemPool : Noncopyable {
 public:
  explicit MemPool()
      : _numaId(-1),
        _safeMt(true),
        _blkAlign(MP_CACHE_LINE_ALIGN),
        _flags(0),
        _mutex() {
    addDefMemSlab();
  }
  explicit MemPool(const int numaId, const uint32_t blkAlign,
                   bool safeMt = true)
      : _numaId(numaId),
        _safeMt(safeMt),
        _blkAlign(blkAlign),
        _flags(0),
        _mutex() {
    addDefMemSlab();
  }
  ~MemPool();

  void setNumaId(const int numaId) { _numaId = numaId; }
  const int numaId() const { return _numaId; }
  void setBlkAlign(const uint32_t blkAlign) { _blkAlign = blkAlign; }
  const uint32_t blkAlign() const { return _blkAlign; }

  bool init();
  void* alloc(const uint32_t size);
  void free(const void* ptr);
  void reduceBlk();

 private:
  int _numaId;
  bool _safeMt; /* Mutithread support */

  uint32_t _blkAlign;
  uint32_t _maxSlabs;
  std::vector<struct MemSlab> _slabs;
  uint32_t _flags;

  /* Use memblk buffer addr to find slab,
   * then put memblk to freelist */
  std::map<uint64_t, uint64_t> _slabBuffMap;
  std::mutex _mutex;

  bool createSlab();
  bool destroySlab();

  inline enum MemBlkIdx getMemBlkIdx(const uint32_t size);
  void addMemSlab(enum MemBlkIdx blkIdx, uint32_t minBlkNR, uint32_t maxBlkNR,
                  uint32_t growBlkNR);
  void addDefMemSlab();
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

}  // namespace BASE
}  // namespace MSF
#endif