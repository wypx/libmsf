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
#ifndef BASE_FILE_UTILS_H_
#define BASE_FILE_UTILS_H_

#pragma once

#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>

#include <limits>
#include <system_error>
#include <vector>

namespace MSF {

/**
 * Convenience wrappers around some commonly used system calls.  The *NoInt
 * wrappers retry on EINTR.  The *Full wrappers retry on EINTR and also loop
 * until all data is written.  Note that *Full wrappers weaken the thread
 * semantics of underlying system calls.
 */
int openNoInt(const char* name, int flags, mode_t mode = 0666);

int closeNoInt(int fd);
int dupNoInt(int fd);
int dup2NoInt(int oldfd, int newfd);
int fsyncNoInt(int fd);
int fdatasyncNoInt(int fd);
int ftruncateNoInt(int fd, off_t len);
int truncateNoInt(const char* path, off_t len);
int flockNoInt(int fd, int operation);
int shutdownNoInt(int fd, int how);

ssize_t readNoInt(int fd, void* buf, size_t n);
ssize_t preadNoInt(int fd, void* buf, size_t n, off_t offset);
ssize_t readvNoInt(int fd, const iovec* iov, int count);

ssize_t writeNoInt(int fd, const void* buf, size_t n);
ssize_t pwriteNoInt(int fd, const void* buf, size_t n, off_t offset);
ssize_t writevNoInt(int fd, const iovec* iov, int count);

/**
 * Wrapper around read() (and pread()) that, in addition to retrying on
 * EINTR, will loop until all data is read.
 *
 * This wrapper is only useful for blocking file descriptors (for non-blocking
 * file descriptors, you have to be prepared to deal with incomplete reads
 * anyway), and only exists because POSIX allows read() to return an incomplete
 * read if interrupted by a signal (instead of returning -1 and setting errno
 * to EINTR).
 *
 * Note that this wrapper weakens the thread safety of read(): the file pointer
 * is shared between threads, but the system call is atomic.  If multiple
 * threads are reading from a file at the same time, you don't know where your
 * data came from in the file, but you do know that the returned bytes were
 * contiguous.  You can no longer make this assumption if using readFull().
 * You should probably use pread() when reading from the same file descriptor
 * from multiple threads simultaneously, anyway.
 *
 * Note that readvFull and preadvFull require iov to be non-const, unlike
 * readv and preadv.  The contents of iov after these functions return
 * is unspecified.
 */
ssize_t readFull(int fd, void* buf, size_t n);
ssize_t preadFull(int fd, void* buf, size_t n, off_t offset);
ssize_t readvFull(int fd, iovec* iov, int count);
ssize_t preadvFull(int fd, iovec* iov, int count, off_t offset);

/**
 * Similar to readFull and preadFull above, wrappers around write() and
 * pwrite() that loop until all data is written.
 *
 * Generally, the write() / pwrite() system call may always write fewer bytes
 * than requested, just like read().  In certain cases (such as when writing to
 * a pipe), POSIX provides stronger guarantees, but not in the general case.
 * For example, Linux (even on a 64-bit platform) won't write more than 2GB in
 * one write() system call.
 *
 * Note that writevFull and pwritevFull require iov to be non-const, unlike
 * writev and pwritev.  The contents of iov after these functions return
 * is unspecified.
 *
 * These functions return -1 on error, or the total number of bytes written
 * (which is always the same as the number of requested bytes) on success.
 */
ssize_t writeFull(int fd, const void* buf, size_t n);
ssize_t pwriteFull(int fd, const void* buf, size_t n, off_t offset);
ssize_t writevFull(int fd, iovec* iov, int count);
ssize_t pwritevFull(int fd, iovec* iov, int count, off_t offset);

/**
 * Read entire file (if num_bytes is defaulted) or no more than
 * num_bytes (otherwise) into container *out. The container is assumed
 * to be contiguous, with element size equal to 1, and offer size(),
 * reserve(), and random access (e.g. std::vector<char>, std::string,
 * fbstring).
 *
 * Returns: true on success or false on failure. In the latter case
 * errno will be set appropriately by the failing system primitive.
 */
template <class Container>
bool readFile(int fd, Container& out,
              size_t num_bytes = std::numeric_limits<size_t>::max()) {
  static_assert(sizeof(out[0]) == 1,
                "readFile: only containers with byte-sized elements accepted");

  size_t soFar = 0;  // amount of bytes successfully read
  {
    assert(out.size() >= soFar);  // resize better doesn't throw
    out.resize(soFar);
  };

  // Obtain file size:
  struct stat buf;
  if (fstat(fd, &buf) == -1) {
    return false;
  }

  // Some files (notably under /proc and /sys on Linux) lie about
  // their size, so treat the size advertised by fstat under advise
  // but don't rely on it. In particular, if the size is zero, we
  // should attempt to read stuff. If not zero, we'll attempt to read
  // one extra byte.
  constexpr size_t initialAlloc = 1024 * 4;
  out.resize(std::min(
      buf.st_size > 0 ? (size_t(buf.st_size) + 1) : initialAlloc, num_bytes));

  while (soFar < out.size()) {
    const auto actual = readFull(fd, &out[soFar], out.size() - soFar);
    if (actual == -1) {
      return false;
    }
    soFar += actual;
    if (soFar < out.size()) {
      // File exhausted
      break;
    }
    // Ew, allocate more memory. Use exponential growth to avoid
    // quadratic behavior. Cap size to num_bytes.
    out.resize(std::min(out.size() * 3 / 2, num_bytes));
  }

  return true;
}

/**
 * Same as above, but takes in a file name instead of fd
 */
template <class Container>
bool readFile(const char* file_name, Container& out,
              size_t num_bytes = std::numeric_limits<size_t>::max()) {
  assert(file_name);

  const auto fd = openNoInt(file_name, O_RDONLY | O_CLOEXEC);
  if (fd == -1) {
    return false;
  }

  {
    // Ignore errors when closing the file
    closeNoInt(fd);
  };

  return readFile(fd, out, num_bytes);
}

/**
 * Writes container to file. The container is assumed to be
 * contiguous, with element size equal to 1, and offering STL-like
 * methods empty(), size(), and indexed access
 * (e.g. std::vector<char>, std::string, fbstring, StringPiece).
 *
 * "flags" dictates the open flags to use. Default is to create file
 * if it doesn't exist and truncate it.
 *
 * Returns: true on success or false on failure. In the latter case
 * errno will be set appropriately by the failing system primitive.
 *
 * Note that this function may leave the file in a partially written state on
 * failure.  Use writeFileAtomic() if you want to ensure that the existing file
 * state will be unchanged on error.
 */
template <class Container>
bool writeFile(const Container& data, const char* filename,
               int flags = O_WRONLY | O_CREAT | O_TRUNC, mode_t mode = 0666) {
  static_assert(sizeof(data[0]) == 1,
                "writeFile works with element size equal to 1");
  int fd = open(filename, flags, mode);
  if (fd == -1) {
    return false;
  }
  bool ok = data.empty() || writeFull(fd, &data[0], data.size()) ==
                                static_cast<ssize_t>(data.size());
  return closeNoInt(fd) == 0 && ok;
}

// for logging
// For passing C-style string argument to a function.
class StringArg {
 public:
  StringArg(const char* str) : m_str(str) {}

  StringArg(const std::string& str) : m_str(str.c_str()) {}

  const char* c_str() const { return m_str; }

 private:
  const char* m_str;
};

class T {
 public:
  T(const char* str, int len) : str_(str), len_(len) {}

  const char* str_;
  const size_t len_;
};

class AppendFile {
 public:
  explicit AppendFile(StringArg filePath);

  ~AppendFile();

  void append(const char* logline, const size_t len);

  size_t write(const char* logline, const size_t len);

  void appendBatch(std::vector<T>::const_iterator buffer1,
                   std::vector<T>::const_iterator end);

  void flush();

  FILE* fp() { return m_fp; }

  off_t writtenBytes() const { return m_writtenBytes; }

 private:
  static const size_t kFileBufferSize = 4096;
  size_t write(const char* logline, int len);

  FILE* m_fp;
  off_t m_writtenBytes;
  char m_buffer[kFileBufferSize];
};

long GetAvailableSpace(const char* path);

}  // namespace MSF
#endif