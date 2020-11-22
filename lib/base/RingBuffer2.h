// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_RING_BUFFER_H_
#define LIB_RING_BUFFER_H_

// https://www.boost.org/doc/libs/1_60_0/doc/html/atomic/usage_examples.html#boost_atomic.usage_examples.example_ringbuffer.implementation

#include <stddef.h>

#include <atomic>

namespace udisk {
namespace blockfs {

template <typename T, int32_t Size>
class RingBuffer {
 public:
  RingBuffer() : head_(0), tail_(0) {}

  bool push(const T& value) {
    int32_t head = head_.load(std::memory_order_relaxed);
    int32_t next_head = next(head);
    if (next_head == tail_.load(std::memory_order_acquire)) return false;
    ring_[head] = value;
    head_.store(next_head, std::memory_order_release);
    return true;
  }
  bool pop(T& value) {
    int32_t tail = tail_.load(std::memory_order_relaxed);
    if (tail == head_.load(std::memory_order_acquire)) return false;
    value = ring_[tail];
    tail_.store(next(tail), std::memory_order_release);
    return true;
  }

 private:
  int32_t next(int32_t current) { return (current + 1) % Size; }
  T ring_[Size];
  std::atomic<int32_t> head_, tail_;
};

}  // namespace blockfs
}  // namespace udisk
#endif