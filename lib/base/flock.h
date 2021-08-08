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
#ifndef BASE_FLOCK_H_
#define BASE_FLOCK_H_

#include <cassert>
#include <condition_variable>
#include <mutex>
#include <string>

#include "noncopyable.h"
namespace MSF {
class FileLock : noncopyable {
 public:
  FileLock() = default;
  FileLock(const std::string &path) : file_name_(path) {}
  virtual ~FileLock() {}
  virtual bool lock(bool create = false) { return false; }
  virtual bool LockAll(bool create = false) { return false; }
  virtual bool unlock() { return false; }

 protected:
  int32_t fd_ = -1;
  std::string file_name_;
};

class PosixFileLock : public FileLock {
 public:
  PosixFileLock() = default;
  PosixFileLock(const std::string &path) : FileLock(path) {}
  ~PosixFileLock();
  bool lock(bool create = false) override;
  bool LockAll(bool create = false) override;
  bool LockRead(uint32_t start, uint32_t whence, uint32_t len);
  bool LockWrite();
  bool TryLockRead(uint32_t start, uint32_t whence, uint32_t len);
  bool TryLockWrite();
  bool unlock() override;
};

// http://blog.sina.com.cn/s/blog_48d4cf2d0100mx6w.html
// https://blog.csdn.net/mymodian9612/article/details/52794980
// Write-first read-write lock
class FileRwLock {
 public:
  FileRwLock() = default;
  ~FileRwLock() = default;

  void ReadLockAcquire() {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_read_.wait(lock, [=]() -> bool {
      return (write_count_ == 0) && (wait_write_count_ == 0);
    });
    ++read_count_;
  }

  // Multiple threads/readers can read within writing at the same time.
  void ReadLockRelease() {
    std::unique_lock<std::mutex> lock(mutex_);
    assert(read_count_ > 0);
    --read_count_;
    if ((read_count_ == 0) && wait_write_count_ > 0) {
      cond_write_.notify_one();
    }
  }

  // There can be no read/write request when want to write
  void WriteLockAcquire() {
    std::unique_lock<std::mutex> lock(mutex_);
    ++wait_write_count_;
    cond_write_.wait(lock, [=]() -> bool {
      return (write_count_ == 0) && (read_count_ == 0);
    });
    assert(write_count_ == 0);
    ++write_count_;
    --wait_write_count_;
  }

  // Only one thread/writer can be allowed
  void WriteLockRelease() {
    std::unique_lock<std::mutex> lock(mutex_);
    assert(write_count_ == 1);
    --write_count_;
    if (wait_write_count_ == 0) {
      cond_read_.notify_all();
    } else {
      cond_write_.notify_one();
    }
  }

 private:
  mutable std::mutex mutex_;
  volatile int32_t wait_write_count_ = 0;
  volatile int32_t write_count_ = 0;
  volatile int32_t read_count_ = 0;
  std::condition_variable cond_write_;
  std::condition_variable cond_read_;
};
}  // namespace MSF
#endif