
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
#include "iouring_poller.h"

#include <assert.h>
#include <base/logging.h>

#include "event.h"
#include "gcc_attr.h"
#include "sock_utils.h"
#include "utils.h"

using namespace MSF;

namespace MSF {

IOUringPoller::IOUringPoller(EventLoop* loop) : Poller(loop) {
  struct io_uring_params params;
  ::memset(&params, 0, sizeof(params));
  if (io_uring_queue_init_params(kEventNumber, &ring_, &params) < 0) {
    LOG(ERROR) << "io_uring_init_failed, quiting";
    ::exit(1);
  }
  // check if IORING_FEAT_FAST_POLL is supported
  if (!(params.features & IORING_FEAT_FAST_POLL)) {
    LOG(ERROR) << "IORING_FEAT_FAST_POLL not available in the kernel, quiting";
    ::exit(0);
  }
  // check if buffer selection is supported
  struct io_uring_probe* probe = io_uring_get_probe_ring(&ring_);
  if (!probe || !io_uring_opcode_supported(probe, IORING_OP_PROVIDE_BUFFERS)) {
    printf("Buffer select not supported, skipping");
    ::exit(0);
  }
  ::free(probe);

  // register buffers for buffer selection
  struct io_uring_sqe* sqe;
  struct io_uring_cqe* cqe;

  sqe = io_uring_get_sqe(&ring_);
  io_uring_prep_provide_buffers(sqe, bufs_, MAX_MESSAGE_LEN, BUFFERS_COUNT,
                                group_id_, 0);

  io_uring_submit(&ring_);
  io_uring_wait_cqe(&ring_, &cqe);
  if (cqe->res < 0) {
    LOG(ERROR) << "cqe->res = " << cqe->res;
    ::exit(1);
  }
  io_uring_cqe_seen(&ring_, cqe);
}

IOUringPoller::~IOUringPoller() { io_uring_queue_exit(&ring_); }

bool IOUringPoller::supported() {
  struct io_uring ring;
  int ret = io_uring_queue_init(16, &ring, 0);
  if (ret) {
    return false;
  }
  io_uring_queue_exit(&ring);
  return true;
}

bool IOUringPoller::AddEvent(const Event* ev) {
  struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
  if (unlikely(ev->events() & Event::kAcceptEvent)) {
    struct sockaddr_storage client_addr;
    socklen_t client_len = sizeof(client_addr);
    // add first accept SQE to monitor for new incoming connections
    io_uring_prep_accept(sqe, ev->fd(), (struct sockaddr*)&client_addr,
                         &client_len, 0);
  } else if (ev->events() & Event::kReadEvent) {
    io_uring_prep_recv(sqe, ev->fd(), nullptr, MAX_MESSAGE_LEN, 0);
    io_uring_sqe_set_flags(sqe, IOSQE_BUFFER_SELECT);
    sqe->buf_group = group_id_;
  } else if (ev->events() & Event::kWriteEvent) {
    // bytes have been read into bufs, now add write to socket sqe
    struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
    io_uring_prep_send(sqe, ev->fd(), &bufs_[ev->bid()], ev->read_bytes(), 0);
    io_uring_sqe_set_flags(sqe, IOSQE_BUFFER_SELECT);
  }
  sqe->user_data = reinterpret_cast<uint64_t>(ev);

  LOG(TRACE) << "Epoll Add"
             << " on " << ev->fd() << " succeeded.";
  return true;
}

bool IOUringPoller::ModifyEvent(const Event* ev) {

  LOG(TRACE) << "Epoll MOD"
             << " on " << ev->fd() << " succeeded.";
  return true;
}

bool IOUringPoller::DeleteEvent(const Event* ev) {

  LOG(TRACE) << "Epoll Del"
             << " on " << ev->fd() << " succeeded.";
  return true;
}

void IOUringPoller::AddProvideBuf(struct io_uring* ring, uint16_t bid,
                                  uint32_t gid, Event* ev) {
  struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
  io_uring_prep_provide_buffers(sqe, bufs_[bid], MAX_MESSAGE_LEN, 1, gid, bid);
  sqe->user_data = reinterpret_cast<uint64_t>(ev);
}

int IOUringPoller::Poll(int timeout_ms, EventList* activeevents_) {
  io_uring_submit_and_wait(&ring_, 1);
  struct io_uring_cqe* cqe;
  uint32_t head;
  uint32_t count = 0;

  // go through all CQEs
  io_uring_for_each_cqe(&ring_, head, cqe) {
    if (cqe->res == -ENOBUFS) {
      LOG(FATAL)
          << "bufs in automatic buffer selection empty, this should not happen";
    }

    ++count;
    Event* ev = reinterpret_cast<Event*>(cqe->user_data);

    if (unlikely(ev->events() & Event::kAcceptEvent)) {
      int fd = cqe->res;
      // only read when there is no error, >= 0
      if (fd >= 0) {
        // Event new_ev(loop_, fd);
        // AddEvent(ev);
      }
      // new connected client; read data from socket
      // and re-add accept to monitor for new connections
      AddEvent(ev);
    } else if (ev->events() & Event::kReadEvent) {
      if (cqe->res <= 0) {
        // read failed, re-add the buffer
        AddProvideBuf(&ring_, ev->bid(), group_id_, ev);
        // connection closed or error
        ::shutdown(ev->fd(), SHUT_RDWR);
      } else {
        ev->set_read_bytes(cqe->res);

        // https://lequ7.com/yuan-sheng-de-Linux-yi-bu-wen-jian-cao-zuo-iouring-chang-xian-ti-yan.html
        // io_uring_prep_readv
        // io_uring_prep_writev
        // io_uring_sqe_set_data
        // io_uring_submit
        // io_uring_peek_cqe
        // io_uring_wait_cqe

        //
        ev->set_events(Event::kWriteEvent);
        // bytes have been read into bufs, now add write to socket sqe
        AddEvent(ev);
      }
    } else if (ev->events() & Event::kWriteEvent) {
      // write has been completed, first re-add the buffer
      AddProvideBuf(&ring_, ev->bid(), group_id_, ev);
      // add a new read for the existing connection
      ev->set_events(Event::kReadEvent);
      AddEvent(ev);
    }
    io_uring_cq_advance(&ring_, count);
  }

  // FillActiveEvents(numevents, activeevents_);
  return 0;
}

void IOUringPoller::FillActiveEvents(int numevents, EventList* active_events) {
  // assert(implicit_cast<size_t>(numevents) <= ep_events_.size());
  /*
   * Notice:
   * Epoll event struct's "data" is a union,
   * one of data.fd and data.ptr is valid.
   * refer:
   * https://blog.csdn.net/sun_z_x/article/details/22581411
   */
  for (int i = 0; i < numevents; ++i) {
    Event* ev = static_cast<Event*>(ep_events_[i].data.ptr);
#ifndef NDEBUG
    int fd = ev->fd();
    auto it = events_.find(fd);
    assert(it != events_.end());
    assert(it->second == ev);
#endif
    ev->set_revents(ep_events_[i].events);
    active_events->push_back(ev);
  }
}

void IOUringPoller::UpdateEvent(Event* ev) {
  const int index = ev->index();
  LOG(TRACE) << "fd = " << ev->fd() << " events = " << ev->events()
             << " index = " << index;

  // Poller::assertInLoopThread();

  if (index == kNew || index == kDeleted) {
    // a new one, add with EPOLL_CTL_ADD
    int fd = ev->fd();
    if (index == kNew) {
      assert(events_.find(fd) == events_.end());
      events_[fd] = ev;
    } else  // index == kDeleted
    {
      assert(events_.find(fd) != events_.end());
      assert(events_[fd] == ev);
    }

    ev->set_index(kAdded);
    AddEvent(ev);
  } else {
    // update existing one with EPOLL_CTL_MOD/DEL
    int fd = ev->fd();
    (void)fd;
    assert(events_.find(fd) != events_.end());
    assert(events_[fd] == ev);
    assert(index == kAdded);
    if (ev->IsNoneEvent()) {
      DeleteEvent(ev);
      ev->set_index(kDeleted);
    } else {
      ModifyEvent(ev);
    }
  }
}
void IOUringPoller::RemoveEvent(Event* ev) {
  Poller::AssertInLoopThread();
  int fd = ev->fd();
  if (unlikely(events_.find(fd) == events_.end())) {
    LOG(ERROR) << "Not found in event map, fd: " << fd;
    return;
  }
  if (unlikely(events_[fd] != ev)) {
    LOG(ERROR) << "Event: " << ev << " and fd: " << fd << " not match";
    return;
  }
  if (unlikely(ev->IsNoneEvent())) {
    LOG(ERROR) << "Event: " << ev << " and fd: " << fd << " has none event";
    return;
  }
  int index = ev->index();
  assert(index == kAdded || index == kDeleted);
  size_t n = events_.erase(fd);
  (void)n;
  assert(n == 1);

  if (index == kAdded) {
    DeleteEvent(ev);
  }
  ev->set_index(kNew);
}

const char* IOUringPoller::OperationToString(int op) {
  switch (op) {
    case EPOLL_CTL_ADD:
      return "ADD";
    case EPOLL_CTL_DEL:
      return "DEL";
    case EPOLL_CTL_MOD:
      return "MOD";
    default:
      assert(false && "ERROR op");
      return "Unknown Operation";
  }
}

}  // namespace MSF