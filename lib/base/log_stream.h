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
#ifndef LIB_LOG_STREAM_H_
#define LIB_LOG_STREAM_H_

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <atomic>
#include <vector>
#include <sched.h>
#include <pthread.h>
#include <assert.h>
#include "utils.h"
#include "noncopyable.h"

namespace MSF {

const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000 * 1000;

template <int SIZE>
class FixedBuffer : Noncopyable {
 public:
  FixedBuffer() : cur_(data_) { setCookie(cookieStart); }

  ~FixedBuffer() { setCookie(cookieEnd); }

  void append(const char* /*restrict*/ buf, size_t len) {
    // FIXME: append partially
    if (implicit_cast<size_t>(avail()) > len) {
      ::memcpy(cur_, buf, len);
      cur_ += len;
    }
  }

  const char* data() const { return data_; }
  int length() const { return static_cast<int>(cur_ - data_); }

  // write to data_ directly
  char* current() { return cur_; }
  int avail() const { return static_cast<int>(end() - cur_); }
  void add(size_t len) { cur_ += len; }

  void reset() { cur_ = data_; }
  void bzero() { ::memset(data_, 0, sizeof data_); }

  // for used by GDB
  const char* debugString();
  void setCookie(void (*cookie)()) { cookie_ = cookie; }
  // for used by unit test
  std::string toString() const { return std::string(data_, length()); }

 private:
  const char* end() const { return data_ + sizeof data_; }
  // Must be outline function for cookies.
  static void cookieStart();
  static void cookieEnd();

  void (*cookie_)();
  char data_[SIZE];
  char* cur_;
};

template <int SIZE>
class CircularBufferTemplate {
 public:
  CircularBufferTemplate() {
    capacity = SIZE;
    capacity_mask = capacity - 1;
    bzero();
  }

  // 需要inline
  bool append(const char* data, size_t len, size_t& remain) {
    // 将data压入,成功或者失败
    // remain代表还剩下多少

    assert(data != nullptr && len > 0);

    // 多写单读的队列,所以这里只有一个读者,但是多个写者
    // 使用了barrier,所以不需要自旋等待其它写者写完
    auto origin_start = write_count.fetch_add(len);
    auto alloc_end = origin_start + len;

    if (capacity >= (alloc_end - read_count)) {
      remain = capacity - (alloc_end - read_count);
      copyToBuffer_(origin_start, alloc_end, data, len);
      return true;
    } else {
      // 空间不够了,push失败,交给外界来处理
      // 将原来占用的空间全部变成0,否则会在文件里面写入其它字符串
      setZero_(origin_start, len);
      write_count.fetch_sub(len);
      return false;
    }
  }

  char* peek(size_t& len) {
    // 这里只有wirte_count存在并发变化(增加)的可能, 因此read_count <
    // write_count肯定是成立的
    if (read_count < write_count) {
      auto read_avail = write_count - read_count;
      auto start = read_count & capacity_mask;
      auto end = (read_count + read_avail) & capacity_mask;

      if (start >= end) {
        read_avail = capacity - start;
      }
      len = static_cast<size_t>(read_avail);
      assert(len <= capacity);
      return buffer + start;  //读取的开始位置
    }

    len = 0;
    return nullptr;
  }

  void pop(size_t len) {
    assert(read_count + len <= write_count);
    read_count += len;
    // idle_count.fetch_add(len);
  }

  void bzero() {
    // 这里不需要整个memcpy 0,因为不会整个Buffer都拿出去写到文件里面的
    // ----------------|--------------------------------|-----------------|
    //      read    read_count       written       write_count alloc_count
    // alloc_count = 0;
    write_count = 0;
    read_count = 0;
    // idle_count = capacity;
  }

  void reset() { bzero(); }

  size_t length() { return capacity - (write_count - read_count); }

 private:
  void copyToBuffer_(size_t start, size_t end, const char* data, int len) {
    auto real_start = start & capacity_mask;
    auto real_end = end & capacity_mask;
    if (real_start < real_end) {
      // 主要是缓存命中的问题导致的？？？？？
      // 我们每次会访问buffer和buffer + real_start,这个有会导致问题???
      // cache miss???
      char* cur = buffer + real_start;
      memcpy(cur, data, len);
    } else {
      size_t first_len = capacity - real_start;
      memcpy(buffer + real_start, data, first_len);
      memcpy(buffer, data, real_end);
    }
  }

  void setZero_(size_t origin_start, size_t len) {

    auto overflow_len = (origin_start + len - read_count) - capacity;
    if (len > overflow_len) {
      // 说明这一次write_count的分配是占用了实际长度的,否则直接减即可
      auto real_len =
          read_count + capacity - origin_start;  // len - overflow_len
      assert(real_len == len - overflow_len);

      auto real_start = origin_start & capacity_mask;
      auto real_end = (origin_start + real_len) & capacity_mask;

      if (real_start < real_end) {
        memset(static_cast<void*>(buffer + real_start), '\0', real_len);
      } else {
        size_t first_len = capacity - real_start;
        memset(static_cast<void*>(buffer + real_start), '\0', first_len);
        memset(static_cast<void*>(buffer + 0), '\0', real_end);
      }
    }
  }

 public:
// 下面这个宏是用来定义变量避免false share的
//#define insert_dummy() [[gnu::aligned(64)]]
#define insert_dummy()

  // std::atomic_ullong  alloc_count insert_dummy();// 到目前为止分配了多少
  std::atomic_uint64_t write_count;  // 到目前为止写入了多少
  // std::atomic_llong idle_count insert_dummy();//
  // 到目前为止还有多少,注意这里不能是无符号,否则会溢出
  uint64_t read_count insert_dummy();  // 到目前为止读取了多少 write - read
  size_t capacity;
  size_t capacity_mask;
  char buffer[SIZE];
};

class BaseBarrier {
 public:
  using StateVec = std::vector<std::atomic_uint32_t>;
  static constexpr uint32_t WRITE_MASK = 1 << 30;
  static constexpr uint32_t READ_MASK = (1 << 16) - 1;
  static constexpr uint32_t IDLE = 0;
};

class ReadBarrier : public BaseBarrier {
 public:
  explicit ReadBarrier(StateVec::value_type& state) : state_(state) {
    // 写优先
    uint32_t old;
    // 持续循环的条件是 等于write 或者有WRITE在等待 或者 更新失败
    while (true) {
      old = state;
      if (old & WRITE_MASK) {
        // 这句话非常重要,如果现在有写锁等,立刻切换走,否则双方都无法获取到锁
        // ::pthread_yield_np();
        ::sched_yield();
        continue;
      }

      if (state.compare_exchange_strong(old, old + 1)) {
        break;
      }
    }
  }

  ~ReadBarrier() { state_.fetch_sub(1); }

  StateVec::value_type& state_;
};

class WriteBarrier : public BaseBarrier {
 public:
  explicit WriteBarrier(StateVec& states) : states_(states) {
    // 需要获取所有读锁
    // 写优先,因此先占据一个标志位
    for (auto& state : states) {
      uint32_t old;
      while (true) {
        old = state;
        if ((old & WRITE_MASK) == 0 &&
            state.compare_exchange_strong(old, old | WRITE_MASK)) {
          break;
        }
      }
    }

    // 先加上等待的状态,在获取后面等待状态的时候,可能前面的读者已经释放了,然后再等读者释放;
    // 如果是一个锁一个锁获取,写锁的持有时间会增加 & 阻塞别的读锁
    // 只要还存在读者,就必须wait
    for (auto& state : states) {
      while (state & READ_MASK) {
      }
    }
  }

  ~WriteBarrier() {
    for (auto& state : states_) {
      // assert(state == WRITE_MASK);
      state = IDLE;
    }
  }

  StateVec& states_;
};

class LogStream : Noncopyable {
  typedef LogStream self;

 public:
  typedef FixedBuffer<kSmallBuffer> Buffer;

  self& operator<<(bool v) {
    buffer_.append(v ? "true" : "fasle", 1);
    return *this;
  }

  self& operator<<(short);
  self& operator<<(unsigned short);
  self& operator<<(int);
  self& operator<<(unsigned int);
  self& operator<<(long);
  self& operator<<(unsigned long);
  self& operator<<(long long);
  self& operator<<(unsigned long long);

  self& operator<<(const void*);

  self& operator<<(float v) {
    *this << static_cast<double>(v);
    return *this;
  }
  self& operator<<(double);
  // self& operator<<(long double);

  self& operator<<(char v) {
    buffer_.append(&v, 1);
    return *this;
  }

  // self& operator<<(signed char);
  // self& operator<<(unsigned char);

  self& operator<<(const char* str) {
    if (str) {
      buffer_.append(str, strlen(str));
    } else {
      buffer_.append("(null)", 6);
    }
    return *this;
  }

  self& operator<<(const unsigned char* str) {
    return operator<<(reinterpret_cast<const char*>(str));
  }

  self& operator<<(const std::string& v) {
    buffer_.append(v.c_str(), v.size());
    return *this;
  }

  void append(const char* data, int len) { buffer_.append(data, len); }
  const Buffer& buffer() const { return buffer_; }
  void resetBuffer() { buffer_.reset(); }

 private:
  void staticCheck();

  template <typename T>
  void formatInteger(T);

  Buffer buffer_;

  static const int kMaxNumericSize = 32;
};

// 专门用来格式化微秒: .%06dz, 10086
// 这样写的目的是因为snprint消耗的时间比较多,所以单独处理下
class FmtMicroSeconds {
 public:
  explicit FmtMicroSeconds(int microSeconds) {
    pthread_once(
        &ponce_,
        &FmtMicroSeconds::
             init);  //第一次调用会在init函数内部创建，pthread_once保证该函数只被调用一次！！！！
    append(microSeconds / 1000, microSeconds % 1000);
  }

  const char* data() const { return buf_; }
  int length() const { return length_; }

 private:
  void append(int num1, int num2) {
    char* threeDigits = strArray[num1];
    buf_[1] = threeDigits[0];
    buf_[2] = threeDigits[1];
    buf_[3] = threeDigits[2];

    threeDigits = strArray[num2];
    buf_[4] = threeDigits[0];
    buf_[5] = threeDigits[1];
    buf_[6] = threeDigits[2];

    buf_[7] = 'z';
    buf_[8] = ' ';
    buf_[9] = '\0';
  }

  // 不能是成员函数,否则带有参数this,不符合pthread_once的规范
  static void init() {
    for (int i = 0; i < 10; i++) {
      for (int j = 0; j < 10; j++) {
        for (int k = 0; k < 10; k++) {
          char* cur = FmtMicroSeconds::strArray[i * 100 + j * 10 + k];
          cur[0] = FmtMicroSeconds::chr[i];
          cur[1] = FmtMicroSeconds::chr[j];
          cur[2] = FmtMicroSeconds::chr[k];
        }
      }
    }
  }

 public:
  char buf_[10] = {'.'};
  int length_ = 9;

 private:
  static pthread_once_t ponce_;  // pthread_once的参数
  static char strArray[1000][3];
  static char chr[10];
};

class Fmt  // : noncopyable
    {
 public:
  template <typename T>
  Fmt(const char* fmt, T val);

  const char* data() const { return buf_; }
  int length() const { return length_; }

 private:
  char buf_[32];
  int length_;
};

inline LogStream& operator<<(LogStream& s, const Fmt& fmt) {
  s.append(fmt.data(), fmt.length());
  return s;
}

// Format quantity n in SI units (k, M, G, T, P, E).
// The returned string is atmost 5 characters long.
// Requires n >= 0
std::string formatSI(int64_t n);

// Format quantity n in IEC (binary) units (Ki, Mi, Gi, Ti, Pi, Ei).
// The returned string is atmost 6 characters long.
// Requires n >= 0
std::string formatIEC(int64_t n);

}  // namespace MSF
#endif