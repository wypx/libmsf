//
// Created by lizgao on 8/21/18.
//

#ifndef BASE_OBJ_POOL_H_
#define BASE_OBJ_POOL_H_

#include <butil/logging.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

#include <vector>

namespace MSF {

template <class T>
class ObjPool {
  using value_type = T;

 public:
  ObjPool(int chunk_cnt) : chunk_cnt_(chunk_cnt), bitmap_(chunk_cnt, 0) {
    account_malloc_cnt_ = (uint64_t *)malloc(chunk_cnt_ * sizeof(uint64_t));
    account_free_cnt_ = (uint64_t *)malloc(chunk_cnt_ * sizeof(uint64_t));
    assert(account_malloc_cnt_);
    assert(account_free_cnt_);
    memset(account_malloc_cnt_, 0, chunk_cnt_ * sizeof(uint64_t));
    memset(account_free_cnt_, 0, chunk_cnt_ * sizeof(uint64_t));

    // MAP_HUGETLB
    value_list_ = reinterpret_cast<value_type *>(
        mmap(NULL, sizeof(T) * chunk_cnt_, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    if (value_list_ == MAP_FAILED) {
      ULOG_ERROR << "Failed to mmap " << strerror(errno)
                 << ", size=" << sizeof(T) << "*" << chunk_cnt_;
      value_list_ = nullptr;
      return;
    }
    free_list_ = (value_type **)malloc(sizeof(value_type *) * chunk_cnt_);
    free_list_size_ = 0;
    for (int i = 0; i < chunk_cnt_; ++i) {
      free_list_[free_list_size_++] = &value_list_[i];
    }
  }
  ~ObjPool() {
    if (account_malloc_cnt_) {
      free(account_malloc_cnt_);
    }
    if (account_free_cnt_) {
      free(account_free_cnt_);
    }

    free(free_list_);
    munmap(value_list_, sizeof(T) * chunk_cnt_);
  }

 public:
  template <typename... _Args>
  value_type *Malloc(_Args &&... __args) {
    if (free_list_size_ == 0) {
      ++malloc_failed_cnt_;
      return nullptr;
    }
    auto ele = free_list_[--free_list_size_];
    new (ele) value_type(std::forward<_Args>(__args)...);
    ++malloc_succeed_cnt_;
    if (free_list_size_ < lowest_free_cnt_) {
      lowest_free_cnt_ = free_list_size_;
    }
    assert(MallocCheckAndSet(ele));
    return ele;
  }
  void Free(value_type *p) {
    assert(FreeCheckAndSet(p));
    p->~value_type();
    free_list_[free_list_size_++] = p;
    ++free_cnt_;
  }

  int free_list_size() { return free_list_size_; }

  int chunk_cnt() { return chunk_cnt_; }

  uint64_t malloc_succeed_cnt() { return malloc_succeed_cnt_; }
  uint64_t malloc_failed_cnt() { return malloc_failed_cnt_; }
  uint64_t free_cnt() { return free_cnt_; }

  int32_t lowest_free_cnt() { return lowest_free_cnt_; }

 private:
  int GetIndex(void *p) {
    auto ptr_int = reinterpret_cast<uintptr_t>(p);
    auto ptr_start_int = reinterpret_cast<uintptr_t>(value_list_);
    auto chunk_size_ = sizeof(value_type);
    uintptr_t offset = ptr_int - ptr_start_int;
    if (offset >= (chunk_size_ * chunk_cnt_)) {
      return -1;
    }

    if (offset % chunk_size_ != 0) {
      return -1;
    }
    return offset / chunk_size_;
  }

  bool MallocCheckAndSet(void *p) {
    auto idx = GetIndex(p);
    if (idx < 0) {
      return false;
    }
    if (bitmap_[idx]) {
      return false;
    }
    bitmap_[idx] = 1;
    account_malloc_cnt_[idx]++;
    return true;
  }

  bool FreeCheckAndSet(void *p) {
    auto idx = GetIndex(p);
    if (idx < 0) {
      return false;
    }
    if (!bitmap_[idx]) {
      sleep(3);
      return false;
    }
    bitmap_[idx] = 0;
    account_free_cnt_[idx]++;
    return true;
  }

 private:
  value_type **free_list_;
  int free_list_size_ = 0;

 private:
  value_type *value_list_;
  int chunk_cnt_;
  uint64_t malloc_succeed_cnt_ = 0;
  uint64_t malloc_failed_cnt_ = 0;
  uint64_t free_cnt_ = 0;
  int32_t lowest_free_cnt_ = INT32_MAX;
  std::vector<uint8_t> bitmap_;
  uint64_t *account_malloc_cnt_ = nullptr;
  uint64_t *account_free_cnt_ = nullptr;
};
}  // namespace MSF
#endif  // USTEVENT_OBJ_POOL_H
