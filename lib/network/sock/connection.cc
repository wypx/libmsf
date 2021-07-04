
#include "connection.h"
#include "sock_utils.h"

#include <base/logging.h>

namespace MSF {

Connection::Connection(EventLoop *loop, int fd, bool thread_safe)
    : loop_(loop), fd_(fd) {
  mutex_ = thread_safe ? new std::mutex : nullptr;
}

Connection::~Connection() {}

void Connection::SubmitWriteBufferSafe(void *buffer, size_t size) noexcept {
  struct iovec iov = {buffer, size};
  if (mutex_) {
    std::lock_guard<std::mutex> lock(*mutex_);
    write_pending_queue_.push_back(std::move(iov));
  } else {
    write_pending_queue_.push_back(std::move(iov));
  }
}

void Connection::SubmitWriteIovecSafe(BufferIovec &queue) noexcept {
  if (mutex_) {
    std::lock_guard<std::mutex> lock(*mutex_);
    SubmitWriteIovec(queue);
  } else {
    SubmitWriteIovec(queue);
  }
}

// https://www.icode9.com/content-4-484721.html
void Connection::SubmitWriteIovec(BufferIovec &queue) noexcept {
  if (queue.empty()) {
    LOG(WARNING) << "no write iovec available";
    return;
  }
  if (write_pending_queue_.empty()) {
    write_pending_queue_.swap(queue);
  } else {
    write_pending_queue_.insert(write_pending_queue_.end(),
                                std::make_move_iterator(queue.begin()),
                                std::make_move_iterator(queue.end()));
  }
}

void Connection::UpdateWriteBusyIovecSafe() noexcept {
  if (mutex_) {
    std::lock_guard<std::mutex> lock(*mutex_);
    UpdateWriteBusyIovec();
  } else {
    UpdateWriteBusyIovec();
  }
}

void Connection::UpdateWriteBusyIovec() noexcept {
  if (write_pending_queue_.empty()) {
    return;
  }
  if (write_busy_queue_.empty()) {
    write_busy_queue_.swap(write_busy_queue_);
  } else {
    write_busy_queue_.insert(
        write_busy_queue_.end(),
        std::make_move_iterator(write_pending_queue_.begin()),
        std::make_move_iterator(write_pending_queue_.end()));
  }
}

void Connection::UpdateWriteBusyOffset(uint64_t bytes_send) noexcept {}

}  // namespace MSF