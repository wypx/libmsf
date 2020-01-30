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
#ifndef __MSF_BUFFER_H__
#define __MSF_BUFFER_H__

#include <base/Noncopyable.h>

#include <atomic>
#include <vector>
// #include <barrier>

using namespace MSF::BASE;

namespace MSF {
namespace BASE {

//std::lock_guard<SpinMutex>lk(mut_);
class SpinMutex {
  public:
    SpinMutex() = default;
    ~SpinMutex() = default;
    SpinMutex(const SpinMutex&) = delete;
    SpinMutex& operator=(const SpinMutex&) = delete;
    SpinMutex(SpinMutex&&) = delete;
    SpinMutex& operator=(SpinMutex&&) = delete;

    void lock() {
        while (flag_.test_and_set(std::memory_order_acquire));
    }
    void unlock() { flag_.clear(std::memory_order_release); }
  private:
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

class Buffer : Noncopyable {
  public:
    static const uint64_t kInitialSize = 1024;
    explicit Buffer(const uint64_t capacity);
    const uint64_t usedSize() const { return 0; }
    const uint64_t capacity() const { return buffer_.capacity(); }

    uint64_t readFd(int fd, int* savedErrno);
  private:
    std::vector<char> buffer_;
    uint64_t readerIndex_;
    uint64_t writerIndex_;
};

}  // namespace BASE
}  // namespace MSF
#endif