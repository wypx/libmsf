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
#ifndef BASE_MEM_ZERO_BUFFER_H
#define BASE_MEM_ZERO_BUFFER_H

#include <base/GccAttr.h>
#include <base/mem/MemPool.h>
#include <sys/uio.h>

using namespace MSF::BASE;

namespace MSF {
namespace BASE {

#if 0
class ZeroBuffer
{
  typedef std::shared_ptr<struct iovec> 
  private:
    std::vector<struct iovec> writeIov_;
    uint32_t writeLeftBytes_;
    uint32_t writeIovIdx_;
    uint32_t writeIovOffset_;

    std::vector<struct iovec> readIov_;
    uint32_t readBytes;
    uint32_t readIovIdx_;
    uint32_t sendIovOffset_;

    bool safeMt_; /* Mutithread support */
    std::mutex mutex_;
  private:
    bool sendBuffer(const int fd) {
      std::vector<struct iovec> writeIovTmp;
      uint32_t writeLeftBytes = writeLeftBytes_;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        writeIov_.swap(writeIovTmp);
      }
      int ret;
      do {
        ret = SendMsg(fd, &*writeIovTmp.begin(), writeIovTmp.size(), MSG_NOSIGNAL | MSG_DONTWAIT);
        if (unlikely(ret == -1)) {
          if (err == EINTR || err == EAGAIN || err == EWOULDBLOCK) {
            continue;
          }
          return false;
        } else if (ret == 0) {
          return false;
        } else {
          writeLeftBytes -= ret;
        }
      } while(ret != writeLeftBytes)
    }
    bool recvBuffer(const int fd) {
      const ssize_t n = SendMsg(fd, vec, iovcnt, MSG_NOSIGNAL | MSG_DONTWAIT);
      if (n <= 0) {
        *savedErrno = errno;
      } else {
        retrieve(n);
      }
    }
  public:
    ZeroBuffer(bool safeMt = true) : safeMt_(safeMt) {

    }
    ~ZeroBuffer() {
    }

    void addBuffer(const void* data, const uint32_t size) {
      if (unlikely(!safeMt_)) {
        std::lock_guard<std::mutex> lock(mutex_);
        addBufferNoLock(data, size);
      } else {
        addBufferNoLock(data, size);
      }
    }
    void addBufferNoLock(const void* data, const uint32_t size) {
      struct iovec iov = { data, size };
      writeIov_.push_back(std::move(iov));
    }
   

    /* Maybe need peek header size */
    bool peekBuffer(void *data, const uint32_t size) {
      if (unlikely(readBytes < size)) {
        MSF_INFO << "There is no enough buffer to peek, size:" 
                  << readyBytes << " need: " << size;
        return false;
      }
      memcpy()
      
    }
};
#endif

}  // namespace BASE
}  // namespace MSF
#endif