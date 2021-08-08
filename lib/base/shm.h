
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
#ifndef BASE_SHM_H_
#define BASE_SHM_H_

#include <iostream>

namespace MSF {

/*
共享内存的其实地址开始处数据:ngx_slab_pool_t + 9 *
sizeof(ngx_slab_page_t)(slots_m[]) + pages * sizeof(ngx_slab_page_t)(pages_m[])
+pages*ngx_pagesize(这是实际的数据部分，
每个ngx_pagesize都由前面的一个ngx_slab_page_t进行管理，并且每个ngx_pagesize最前端第一个obj存放的是一个或者多个int类型bitmap，用于管理每块分配出去的内存)
*/
//见ngx_init_zone_pool共享内存的起始地址开始的sizeof(ngx_slab_pool_t)字节是用来存储管理共享内存的slabpoll的

class ShmManager {
 public:
  enum AllocateMethod {
    kAllocateMethodInvalid = 0,
    kAllocateMappingFile,
    kAllocateMappingMem,
    kAllocateMappingDevZero,
    kAllocateSysvShmget,
    kAllocatePosixShmopen
  };

 public:
  ShmManager() = default;
  explicit ShmManager(const std::string &name, uint64_t size);
  virtual ~ShmManager() { Free(); }

  void set_alloc_method(AllocateMethod method) noexcept {
    alloc_method_ = method;
  }
  const AllocateMethod alloc_method() const { return alloc_method_; }

  bool Initialize();
  bool Allocate();
  bool Free();

  bool ReadLock() const;
  bool WriteLock() const;
  bool TryReadLock() const;
  bool TryWriteLock() const;
  bool UnReadLock() const;
  bool UnWriteLock() const;

  bool PosixShmLock(const void *ptr, size_t size);
  bool PosixShmUnLock(const void *ptr, size_t size);

  bool LockReadWrite() const;
  bool UnLockReadWrite() const;
  bool TryLockReadWrite() const;

  static bool MemLock(const void *ptr, size_t size);
  static bool MemUnLock(const void *ptr, size_t size);
  static bool MemProtect(void *ptr, size_t size, int flag);
  static bool MemSync(void *ptr, size_t size, int flag);
  static bool MemLockAll(int flags);
  static bool MemUnLockAll(int flags);

 private:
  std::string mapfile;
  std::string name;
  uint64_t size;
  char *map_addr_;
  AllocateMethod alloc_method_;

  int sem_id_;
  key_t shm_key_;

  int region_size_ = 0;
  std::string shm_name_ = "posix_shm";
  void *posix_shm_addr_ = nullptr;

  bool InitializeSysv();
  bool InitializePosix();
  bool AllocateMappingFile();
  bool AllocateMappingMem();
  bool AllocateMappingDevZero();
  bool AllocateShmget();
  bool AllocateShmopen();
  int ShmAllocByMapFile(const std::string &filename);
};

}  // namespace MSF
#endif
