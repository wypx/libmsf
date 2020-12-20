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
#ifndef BASE_BUFFER_H_
#define BASE_BUFFER_H_

#include <stdint.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <memory>
#include <string>
#include <vector>
// #include <barrier>

#include "byteoder.h"
#include "noncopyable.h"
#include "utils.h"

using namespace MSF;

// https://github.com/lava/linear_ringbuffer/tree/master/include/bev

namespace MSF {

// std::lock_guard<SpinMutex>lk(mut_);
class SpinMutex {
 public:
  SpinMutex() = default;
  ~SpinMutex() = default;
  SpinMutex(const SpinMutex &) = delete;
  SpinMutex &operator=(const SpinMutex &) = delete;
  SpinMutex(SpinMutex &&) = delete;
  SpinMutex &operator=(SpinMutex &&) = delete;

  void lock() {
    while (flag_.test_and_set(std::memory_order_acquire))
      ;
  }
  void unlock() { flag_.clear(std::memory_order_release); }

 private:
  std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

/// 0         readerIndex##############writerIndex              capacity
//  0#########writerIndex              readerIndex##############capacity

class Buffer {
 public:
  static const size_t kInitialSize = 4096;

  explicit Buffer(size_t initialSize = kInitialSize)
      : buffer_(initialSize),
        capacity_(initialSize),
        readerIndex_(0),
        writerIndex_(0) {
    assert(readableBytes() == 0);
    assert(writeableBytes() == initialSize - 1);
  }

  size_t readableBytes() const {
    ssize_t diff = writerIndex_ - readerIndex_;
    if (diff < 0) {
      diff += capacity_;
    }
    return diff;
  }

  // 留一个位置不写, 方便区分写满还是空
  size_t writeableBytes() const { return capacity_ - 1 - readableBytes(); }

  // 移除数据，不发生拷贝
  ssize_t retrieve(size_t len) {
    if (len > readableBytes()) {
      return -1;
    }
    readerIndex_ += len;
    if (readerIndex_ >= capacity_) {
      readerIndex_ -= capacity_;
    }
    return len;
  }

  void retrieveAll() {
    readerIndex_ = 0;
    writerIndex_ = 0;
  }

  //拷贝出数据，不移除数据
  size_t receive(void *data, size_t len) {
    if (len > readableBytes()) {
      return -1;
    }
    size_t next_readerIndex_ = readerIndex_ + len;
    if (next_readerIndex_ < capacity_) {
      ::memcpy(data, &buffer_[readerIndex_], len);
    } else {
      ::memcpy(data, &buffer_[readerIndex_], capacity_ - readerIndex_);
      ::memcpy((char *)data + (capacity_ - readerIndex_), &buffer_[0],
               next_readerIndex_ - capacity_);
    }
    return len;
  }

  //拷贝数据，并且移除
  size_t remove(void *data, size_t len) {
    if (len > readableBytes()) {
      return -1;
    }
    size_t next_readerIndex_ = readerIndex_ + len;
    if (next_readerIndex_ < capacity_) {
      ::memcpy(data, &buffer_[readerIndex_], len);
      readerIndex_ += len;
    } else {
      ::memcpy(data, &buffer_[readerIndex_], capacity_ - readerIndex_);
      ::memcpy((char *)data + (capacity_ - readerIndex_), &buffer_[0],
               next_readerIndex_ - capacity_);
      readerIndex_ = next_readerIndex_ - capacity_;
    }
    return len;
  }

  void hasWritten(size_t len) {
    assert(len <= writeableBytes());
    size_t next_writerIndex_ = writerIndex_ + len;
    if (next_writerIndex_ < capacity_) {
      writerIndex_ += len;
    } else {
      writerIndex_ = next_writerIndex_ - capacity_;
      assert(writerIndex_ < readerIndex_);
    }
  }

  // 追加数据
  void append(const char * /*restrict*/ data, size_t len) {
    if (writeableBytes() < len) {
      makeSpace(len);
    }
    DoAppend(data, len);
  }

  void append(const void * /*restrict*/ data, size_t len) {
    const char *_data = static_cast<const char *>(data);
    if (writeableBytes() < len) {
      makeSpace(len);
    }
    DoAppend(_data, len);
  }

  void DoAppend(const char * /*restrict*/ data, size_t len) {
    assert(len <= writeableBytes());
    size_t next_writerIndex_ = writerIndex_ + len;
    if (next_writerIndex_ < capacity_) {
      ::memcpy(&buffer_[writerIndex_], data, len);
      writerIndex_ += len;
    } else {
      ::memcpy(&buffer_[writerIndex_], data, capacity_ - writerIndex_);
      ::memcpy(&buffer_[0], data + (capacity_ - writerIndex_),
               next_writerIndex_ - capacity_);
      writerIndex_ = next_writerIndex_ - capacity_;
      assert(writerIndex_ < readerIndex_);
    }
  }

  /// Read data directly into buffer.
  ///
  /// It may implement with readv(2)
  /// @return result of read(2), @c errno is saved
  ssize_t ReadFd(int fd, int *savedErrno);

  ssize_t WriteFd(int fd);

 private:
  void makeSpace(size_t len) {
    size_t readable = readableBytes();
    size_t diff = len - writeableBytes();
    capacity_ += diff;
    // LOG(DEBUG) << "buffer size increase:" << diff
    //           << " new capacity:" << capacity_;
    if (readerIndex_ <= writerIndex_) {  //在尾部增加diff 不需要移动数据
      buffer_.resize(capacity_);
    } else {  // 扩大空间, 移动数据，数据分为两段
      std::vector<char> tmp;
      tmp.swap(buffer_);
      size_t part1_size = tmp.size() - readerIndex_;
      size_t part2_size = writerIndex_;
      buffer_.resize(capacity_);
      std::copy(tmp.begin() + readerIndex_, tmp.end(), buffer_.begin());
      std::copy(tmp.begin(), tmp.begin() + writerIndex_,
                buffer_.begin() + part1_size);
      readerIndex_ = 0;
      writerIndex_ = part1_size + part2_size;
    }
    assert(readable == readableBytes());
  }

 private:
  std::vector<char> buffer_;
  size_t capacity_;
  size_t readerIndex_;
  size_t writerIndex_;
};

}  // namespace MSF
#endif