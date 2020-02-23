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
#include <base/Utils.h>
#include <base/Endian.h>

#include <algorithm>
#include <memory>
#include <atomic>
#include <vector>
#include <string>
#include <cassert>
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
    static const size_t kCheapPrepend = 8;
    explicit Buffer(const uint64_t capacity = kInitialSize)
    : buffer_(kCheapPrepend + capacity),
      readerIndex_(kCheapPrepend),
      writerIndex_(kCheapPrepend)
    {
      
    }
    const uint64_t usedSize() const { return 0; }
    const uint64_t capacity() const { return buffer_.capacity(); }

    size_t readableBytes() const
    { return writerIndex_ - readerIndex_; }

    size_t writableBytes() const
    { return buffer_.size() - writerIndex_; }

    size_t prependableBytes() const
    { return readerIndex_; }

    const char* peek() const
    { return begin() + readerIndex_; }

    const char* findCRLF() const
    {
      // FIXME: replace with memmem()?
      const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF+2);
      return crlf == beginWrite() ? NULL : crlf;
    }

    const char* findCRLF(const char* start) const
    {
      assert(peek() <= start);
      assert(start <= beginWrite());
      // FIXME: replace with memmem()?
      const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF+2);
      return crlf == beginWrite() ? NULL : crlf;
    }

    const char* findEOL() const
    {
      const void* eol = memchr(peek(), '\n', readableBytes());
      return static_cast<const char*>(eol);
    }

    const char* findEOL(const char* start) const
    {
      assert(peek() <= start);
      assert(start <= beginWrite());
      const void* eol = memchr(start, '\n', beginWrite() - start);
      return static_cast<const char*>(eol);
    }

    // retrieve returns void, to prevent
    // string str(retrieve(readableBytes()), readableBytes());
    // the evaluation of two functions are unspecified
    void retrieve(size_t len)
    {
      assert(len <= readableBytes());
      if (len < readableBytes())
      {
        readerIndex_ += len;
      }
      else
      {
        retrieveAll();
      }
    }

    void retrieveUntil(const char* end)
    {
      assert(peek() <= end);
      assert(end <= beginWrite());
      retrieve(end - peek());
    }

    void retrieveInt64()
    {
      retrieve(sizeof(int64_t));
    }

    void retrieveInt32()
    {
      retrieve(sizeof(int32_t));
    }

    void retrieveInt16()
    {
      retrieve(sizeof(int16_t));
    }

    void retrieveInt8()
    {
      retrieve(sizeof(int8_t));
    }

    void retrieveAll()
    {
      readerIndex_ = kCheapPrepend;
      writerIndex_ = kCheapPrepend;
    }

    std::string retrieveAllAsString()
    {
      return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len)
    {
      assert(len <= readableBytes());
      std::string result(peek(), len);
      retrieve(len);
      return result;
    }

    void append(const char* /*restrict*/ data, size_t len)
    {
      ensureWritableBytes(len);
      std::copy(data, data+len, beginWrite());
      hasWritten(len);
    }

    void append(const void* /*restrict*/ data, size_t len)
    {
      append(static_cast<const char*>(data), len);
    }

  void ensureWritableBytes(size_t len)
    {
      if (writableBytes() < len)
      {
        makeSpace(len);
      }
      assert(writableBytes() >= len);
    }

    char* beginWrite()
    { return begin() + writerIndex_; }

    const char* beginWrite() const
    { return begin() + writerIndex_; }

    void hasWritten(size_t len)
    {
      assert(len <= writableBytes());
      writerIndex_ += len;
    }

    void unwrite(size_t len)
    {
      assert(len <= readableBytes());
      writerIndex_ -= len;
    }

    ///
    /// Append int64_t using network endian
    ///
    void appendInt64(int64_t x)
    {
      // int64_t be64 = sockets::hostToNetwork64(x);
      int64_t be64 = cpu_to_be64(x);
      append(&be64, sizeof be64);
    }

    ///
    /// Append int32_t using network endian
    ///
    void appendInt32(int32_t x)
    {
      // int32_t be32 = sockets::hostToNetwork32(x);
      int32_t be32 = cpu_to_be32(x);
      append(&be32, sizeof be32);
    }

    void appendInt16(int16_t x)
    {
      // int16_t be16 = sockets::hostToNetwork16(x);
      int16_t be16 = cpu_to_be16(x);
      append(&be16, sizeof be16);
    }

    void appendInt8(int8_t x)
    {
      append(&x, sizeof x);
    }

  ///
    /// Read int64_t from network endian
    ///
    /// Require: buf->readableBytes() >= sizeof(int32_t)
    int64_t readInt64()
    {
      int64_t result = peekInt64();
      retrieveInt64();
      return result;
    }

    ///
    /// Read int32_t from network endian
    ///
    /// Require: buf->readableBytes() >= sizeof(int32_t)
    int32_t readInt32()
    {
      int32_t result = peekInt32();
      retrieveInt32();
      return result;
    }

    int16_t readInt16()
    {
      int16_t result = peekInt16();
      retrieveInt16();
      return result;
    }

    int8_t readInt8()
    {
      int8_t result = peekInt8();
      retrieveInt8();
      return result;
    }

    ///
    /// Peek int64_t from network endian
    ///
    /// Require: buf->readableBytes() >= sizeof(int64_t)
    int64_t peekInt64() const
    {
      assert(readableBytes() >= sizeof(int64_t));
      int64_t be64 = 0;
      ::memcpy(&be64, peek(), sizeof be64);
      // return sockets::networkToHost64(be64);
      return be64_to_cpu(be64);
    }

    ///
    /// Peek int32_t from network endian
    ///
    /// Require: buf->readableBytes() >= sizeof(int32_t)
    int32_t peekInt32() const
    {
      assert(readableBytes() >= sizeof(int32_t));
      int32_t be32 = 0;
      ::memcpy(&be32, peek(), sizeof be32);
      // return sockets::networkToHost32(be32);
      return be32_to_cpu(be32);
    }

    int16_t peekInt16() const
    {
      assert(readableBytes() >= sizeof(int16_t));
      int16_t be16 = 0;
      ::memcpy(&be16, peek(), sizeof be16);
      // return sockets::networkToHost16(be16);
      return be16_to_cpu(be16);
    }

    int8_t peekInt8() const
    {
      assert(readableBytes() >= sizeof(int8_t));
      int8_t x = *peek();
      return x;
    }

    ///
    /// Prepend int64_t using network endian
    ///
    void prependInt64(int64_t x)
    {
      // int64_t be64 = sockets::hostToNetwork64(x);
      int64_t be64 = cpu_to_le64(x);
      prepend(&be64, sizeof be64);
    }

    ///
    /// Prepend int32_t using network endian
    ///
    void prependInt32(int32_t x)
    {
      // int32_t be32 = sockets::hostToNetwork32(x);
      int32_t be32 = cpu_to_le32(x);
      prepend(&be32, sizeof be32);
    }

    void prependInt16(int16_t x)
    {
      // int16_t be16 = sockets::hostToNetwork16(x);
      int16_t be16 = cpu_to_le16(x);
      prepend(&be16, sizeof be16);
    }

    void prependInt8(int8_t x)
    {
      prepend(&x, sizeof x);
    }

    void prepend(const void* /*restrict*/ data, size_t len)
    {
      assert(len <= prependableBytes());
      readerIndex_ -= len;
      const char* d = static_cast<const char*>(data);
      std::copy(d, d+len, begin()+readerIndex_);
    }

    void shrink(size_t reserve)
    {
      // FIXME: use vector::shrink_to_fit() in C++ 11 if possible.
      // Buffer other;
      // other.ensureWritableBytes(readableBytes()+reserve);
      // other.append(toStringPiece());
      // swap(other);
    }

    size_t internalCapacity() const
    {
      return buffer_.capacity();
    }

    /// Read data directly into buffer.
    ///
    /// It may implement with readv(2)
    /// @return result of read(2), @c errno is saved
    // ssize_t readFd(int fd, int* savedErrno);
    uint64_t readFd(const int fd, int* savedErrno);
    uint64_t sendFd(const int fd, int* savedErrno);
  private:

    char* begin()
    { return &*buffer_.begin(); }

    const char* begin() const
    { return &*buffer_.begin(); }

    void makeSpace(size_t len)
    {
      if (writableBytes() + prependableBytes() < len + kCheapPrepend)
      {
        // FIXME: move readable data
        buffer_.resize(writerIndex_+len);
      }
      else
      {
        // move readable data to the front, make space inside buffer
        assert(kCheapPrepend < readerIndex_);
        size_t readable = readableBytes();
        std::copy(begin()+readerIndex_,
                  begin()+writerIndex_,
                  begin()+kCheapPrepend);
        readerIndex_ = kCheapPrepend;
        writerIndex_ = readerIndex_ + readable;
        assert(readable == readableBytes());
      }
    }
  private:
    std::vector<char> buffer_;
    uint64_t readerIndex_;
    uint64_t writerIndex_;

    static const char kCRLF[];
};

}  // namespace BASE
}  // namespace MSF
#endif