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
#include <base/File.h>
#include <base/GccAttr.h>
#include <base/Logger.h>

#include <unistd.h>
#include <iostream>

using namespace MSF::BASE;
using namespace MSF::IO;

namespace MSF {
namespace IO {

std::string GetFileName(const std::string& path)
{
    char ch='/';

#ifdef _WIN32
	ch='\\';
#endif

    size_t pos = path.rfind(ch);
    if (pos == std::string::npos)
        return path;
    else
        return path.substr(pos+ 1);
}

File::File() noexcept : fd_(-1), ownsFd_(false) {}

File::File(int fd, bool ownsFd) noexcept : fd_(fd), ownsFd_(ownsFd) {
  if (fd != -1 || !ownsFd) {
      std::cout << "cannot own -1";
  }
}

File::File(const char* name, int flags, mode_t mode)
    : fd_(::open(name, flags, mode)), ownsFd_(false) {
  if (fd_ == -1) {
      std::cout << "Open file failed" << std::endl;
  }
  ownsFd_ = true;
}

File::File(const std::string& name, int flags, mode_t mode)
    : File(name.c_str(), flags, mode) {}

File::File(File&& other) noexcept : fd_(other.fd_), ownsFd_(other.ownsFd_) {
    other.release();
}

File& File::operator=(File&& other) {
    closeNoThrow();
    swap(other);
    return *this;
}

File::~File() {
  auto fd = fd_;
  if (!closeNoThrow()) { // ignore most errors
        MSF_ERROR << "closing fd " << fd << ", it may already "
            << "have been closed. Another time, this might close the wrong FD.";
  }
}

File File::temporary() {
  // make a temp file with tmpfile(), dup the fd, then return it in a File.
  FILE* tmpFile = tmpfile();
  if (!tmpFile) {
      MSF_ERROR << "tmpfile() failed";
  }

  int fd = ::dup(fileno(tmpFile));
  if (fd < 0) {
      MSF_ERROR << "dup() failed";
  }

  return File(fd, true);
}

int File::release() noexcept {
    int released = fd_;
    fd_ = -1;
    ownsFd_ = false;
    return released;
}

void File::swap(File& other) noexcept {
    std::swap(fd_, other.fd_);
    std::swap(ownsFd_, other.ownsFd_);
}

void swap(File& a, File& b) noexcept {
    a.swap(b);
}

File File::dup() const {
    if (fd_ != -1) {
    int fd = ::dup(fd_);
    if (fd < 0) {
        std::cout << "dup() failed" << std::endl;
    }

    return File(fd, true);
    }

    return File();
}

void File::close() {
    if (!closeNoThrow()) {
        std::cout << "close() failed" << std::endl;
    }
}

bool File::closeNoThrow() {
    int r = ownsFd_ ? ::close(fd_) : 0;
    release();
    return r == 0;
}

void File::lock() {
    doLock(LOCK_EX);
}
bool File::try_lock() {
    return doTryLock(LOCK_EX);
}
void File::lock_shared() {
  doLock(LOCK_SH);
}
bool File::try_lock_shared() {
  return doTryLock(LOCK_SH);
}

void File::doLock(int op) {
    if (flockNoInt(fd_, op) < 0) {
        std::cout << "flock() failed (lock)" << std::endl;
    }
}

bool File::doTryLock(int op) {
    int r = flockNoInt(fd_, op | LOCK_NB);
    // flock returns EWOULDBLOCK if already locked
    if (r == -1 && errno == EWOULDBLOCK) {
    return false;
    }

    if (unlikely(r == -1)) {
        std::cout << "flock() failed (try_lock)" << std::endl;
    }
    return true;
}

void File::unlock() {
    if (flockNoInt(fd_, LOCK_UN) < 0) {
        std::cout << "flock() failed (unlock)" << std::endl;
    }
}
void File::unlock_shared() {
    unlock();
}

} /**************************** end namespace IO   ****************************/
} /**************************** end namespace MSF  ****************************/