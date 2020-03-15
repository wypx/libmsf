#include "MmapFile.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>

static const size_t kDefaultSize = 1 * 1024 * 1024;

static const int kInvalidFile = -1;
static char* const kInvalidAddr = reinterpret_cast<char*>(-1);

// https://blog.csdn.net/gatieme/article/details/50779184

bool mapMem(const uint32_t n) {
  // Technically, we could also report sucess here since a zero-length
  // buffer can't be legally used anyways.
  if (n == 0) {
    errno = EINVAL;
    return false;
  }
  int PAGE_SIZE = 0;
  // Round up to nearest multiple of page size.
	int bytes = n & ~(PAGE_SIZE-1);
	if (n % PAGE_SIZE) {
		bytes += PAGE_SIZE;
	}

	// Check for overflow.
	if (bytes*2u < bytes) {
		errno = EINVAL;
		return -1;
	}

// Use `char*` instead of `void*` because we need to do arithmetic on them.
	uint8_t* addr =nullptr;
	uint8_t* addr2=nullptr;
  // Allocate twice the buffer size
  addr = static_cast<unsigned char*>(mmap(NULL, 2*bytes,
    PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));

  if (addr == MAP_FAILED) {
    goto errout;
  }

  // Shrink to actual buffer size.
  addr = static_cast<uint8_t*>(mremap(addr, 2*bytes, bytes, 0));
  if (addr == MAP_FAILED) {
    goto errout;
  }

  // Create the second copy right after the shrinked buffer.
  addr2 = static_cast<uint8_t*>(mremap(addr, 0, bytes, MREMAP_MAYMOVE,
    addr+bytes));

  if (addr2 == MAP_FAILED) {
    goto errout;
  }

  if (addr2 != addr+bytes) {
    errno = EAGAIN;
    goto errout;
  }
  return true;
errout:
  // We actually have to check for non-null here, since even if `addr` is
  // null, `bytes` might be large enough that this overlaps some actual
  // mappings.
  if (addr) {
    ::munmap(addr, bytes);
  }
  if (addr2) {
    ::munmap(addr2, bytes);
  }
  return false;
}

// OMmapFile
OMmapFile::OMmapFile()
    : file_(kInvalidFile),
      memory_(kInvalidAddr),
      offset_(0),
      size_(0),
      syncPos_(0) {}

OMmapFile::~OMmapFile() { Close(); }

void OMmapFile::_ExtendFileSize(size_t size) {
  assert(file_ != kInvalidFile);

  if (size > size_) Truncate(size);
}

bool OMmapFile::Open(const std::string& file, bool bAppend) {
  return Open(file.c_str(), bAppend);
}

bool OMmapFile::Open(const char* file, bool bAppend) {
  Close();

  file_ = ::open(file, O_RDWR | O_CREAT | (bAppend ? O_APPEND : 0), 0644);

  if (file_ == kInvalidFile) {
    char err[128];
    snprintf(err, sizeof err - 1, "OpenWriteOnly %s failed\n", file);
    perror(err);
    assert(false);
    return false;
  }

  if (bAppend) {
    struct stat st;
    fstat(file_, &st);
    size_ = std::max<size_t>(kDefaultSize, st.st_size);
    offset_ = st.st_size;
  } else {
    size_ = kDefaultSize;
    offset_ = 0;
  }

  int ret = ::ftruncate(file_, size_);
  assert(ret == 0);

  return _MapWriteOnly();
}

void OMmapFile::Close() {
  if (file_ != kInvalidFile) {
    ::munmap(memory_, size_);
    ::ftruncate(file_, offset_);
    ::close(file_);

    file_ = kInvalidFile;
    size_ = 0;
    memory_ = kInvalidAddr;
    offset_ = 0;
    syncPos_ = 0;
  }
}

bool OMmapFile::Sync() {
  if (file_ == kInvalidFile) return false;

  if (syncPos_ >= offset_) return false;

  ::msync(memory_ + syncPos_, offset_ - syncPos_, MS_SYNC);
  syncPos_ = offset_;

  return true;
}

bool OMmapFile::_MapWriteOnly() {
  if (size_ == 0 || file_ == kInvalidFile) {
    assert(false);
    return false;
  }

  memory_ = (char*)::mmap(0, size_, PROT_WRITE, MAP_SHARED, file_, 0);
  return (memory_ != kInvalidAddr);
}

void OMmapFile::Truncate(std::size_t size) {
  if (size == size_) return;

  size_ = size;
  int ret = ::ftruncate(file_, size_);
  assert(ret == 0);

  if (offset_ > size_) offset_ = size_;

  _MapWriteOnly();
}

bool OMmapFile::IsOpen() const { return file_ != kInvalidFile; }

// consumer
void OMmapFile::Write(const void* data, size_t len) {
  _AssureSpace(len);

  assert(offset_ + len <= size_);

  ::memcpy(memory_ + offset_, data, len);
  offset_ += len;
  assert(offset_ <= size_);
}

void OMmapFile::_AssureSpace(size_t len) {
  size_t newSize = size_;

  while (offset_ + len > newSize) {
    if (newSize == 0)
      newSize = kDefaultSize;
    else
      newSize *= 2;
  }

  _ExtendFileSize(newSize);
}