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
#ifndef BASE_SLICE_H_
#define BASE_SLICE_H_

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <stdexcept>
#include <string>

namespace MSF {

class Slice {
 public:
  Slice() : data_(""), size_(0) {}

  Slice(const char* s) : data_(s) {
    assert(data_ != NULL);
    size_ = strlen(s);
  }

  Slice(const char* s, size_t n) : data_(s), size_(n) {}

  Slice(const std::string& s) : data_(s.c_str()), size_(s.size()) {}

  const char* data() const { return data_; }

  size_t size() const { return size_; }

  bool empty() const { return size_ == 0; }

  void resize(size_t s) { size_ = s; }

  void clear() {
    data_ = "";
    size_ = 0;
  }

  char operator[](size_t n) const {
    assert(n < size());
    return data_[n];
  }

  int compare(const Slice& x) const {
    int common_prefix_len = (size_ < x.size_) ? size_ : x.size_;
    int r = memcmp(data_, x.data_, common_prefix_len);
    if (r == 0) {
      if (size_ < x.size_)
        r = -1;
      else if (size_ > x.size_)
        r = 1;
    }
    return r;
  }

  std::string to_string() const { return std::string(data_, size_); }

  static Slice alloc(size_t size) {
    assert(size);
    char* buf = new char[size];
    assert(buf);
    return Slice(buf, size);
  }

  Slice clone() const {
    assert(size_);
    char* buf = new char[size_];
    assert(buf);
    memcpy(buf, data_, size_);
    return Slice(buf, size_);
  }

  void destroy() {
    assert(size_);
    delete[] data_;
    clear();
  }

 private:
  const char* data_;  // start address of buffer
  uint32_t size_;     // buffer length
};

inline bool operator==(const Slice& x, const Slice& y) {
  return x.compare(y) == 0;
}

inline bool operator!=(const Slice& x, const Slice& y) { return !(x == y); }

inline bool operator<(const Slice& x, const Slice& y) {
  return x.compare(y) < 0;
}
}  // namespace MSF

#endif