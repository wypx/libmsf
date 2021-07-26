
#include "connection.h"
#include "sock_utils.h"
#include "event.h"
#include "event_loop.h"

#include <base/logging.h>

namespace MSF {

Connection::Connection(EventLoop *loop, int fd, bool thread_safe)
    : loop_(loop), fd_(fd) {
  event_.reset(new Event(loop, fd));
  mutex_ = thread_safe ? new std::mutex : nullptr;
}

Connection::~Connection() {}

void Connection::AddGeneralEvent() { loop_->UpdateEvent(event_.get()); }

void Connection::AddWriteEvent() {
  uint16_t events = event_->events() | (Event::kWriteEvent);
  event_->set_events(events);
  loop_->UpdateEvent(event_.get());
}

void Connection::RemoveWriteEvent() {
  uint16_t events = event_->events() & (~Event::kWriteEvent);
  event_->set_events(events);
  loop_->UpdateEvent(event_.get());
}

void Connection::RemoveAllEvent() { loop_->RemoveEvent(event_.get()); }

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

bool Connection::UpdateWriteBusyIovecSafe() noexcept {
  if (mutex_) {
    std::lock_guard<std::mutex> lock(*mutex_);
    return UpdateWriteBusyIovec();
  } else {
    return UpdateWriteBusyIovec();
  }
}

bool Connection::UpdateWriteBusyIovec() noexcept {
  if (write_pending_queue_.empty()) {
    return false;
  }
  // if (write_busy_queue_.empty()) {
  //   write_busy_queue_.swap(write_pending_queue_);
  // } else {

  // }

  write_busy_queue_.insert(
      write_busy_queue_.end(),
      std::make_move_iterator(write_pending_queue_.begin()),
      std::make_move_iterator(write_pending_queue_.end()));

  write_pending_queue_.clear();

  LOG(TRACE) << "pendding size: " << write_pending_queue_.size()
             << " busy size: " << write_busy_queue_.size();
  return write_busy_queue_.empty();
}

void Connection::UpdateWriteBusyOffset(uint64_t bytes_send) noexcept {}

void Connection::ClearWritePendingIovec() noexcept {
  write_pending_queue_.clear();
}

void Connection::ClearWriteBusyIovec() noexcept { write_busy_queue_.clear(); }

}  // namespace MSF