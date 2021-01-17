#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <atomic>
#include <iostream>
#include <memory>
#include <vector>

namespace MSF {

template <typename T>
class RingBuffer {
 public:
  RingBuffer(unsigned capacity = 60)
      : buffer_(capacity), capacity_(capacity), num_(0) {}

  ~RingBuffer() {}

  bool push(const T& data) { return pushData(std::forward<T>(data)); }
  bool push(T&& data) { return pushData(data); }

  bool pop(T& data) {
    if (num_ > 0) {
      data = std::move(buffer_[head_]);
      add(head_);
      num_--;
      return true;
    }
    return false;
  }

  bool isFull() const { return ((num_ == capacity_) ? true : false); }
  bool isEmpty() const { return ((num_ == 0) ? true : false); }
  int size() const { return num_; }

 private:
  template <typename F>
  bool pushData(F&& data) {
    if (num_ < capacity_) {
      buffer_[tail_] = std::forward<F>(data);
      add(tail_);
      num_++;
      return true;
    }

    return false;
  }

  void add(int& pos) { pos = (((pos + 1) == capacity_) ? 0 : (pos + 1)); }

  int capacity_ = 0;
  int head_ = 0;
  int tail_ = 0;

  std::atomic_int num_;
  std::vector<T> buffer_;
};

}  // namespace MSF
