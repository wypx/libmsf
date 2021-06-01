#include "aio_context.h"

#include <base/logging.h>

#if 0
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <system_error>
#include <vector>

AIOContext::AIOContext(std::size_t _queue_depth)
    : aio_ctx(0), queue_depth(_queue_depth), inflight_requests(0) {
  int error_code = io_setup(queue_depth, &aio_ctx);
  if (error_code) {
    throw std::system_error(-error_code, std::system_category(),
                            "failed to set up the AIO context");
  }
}

AIOContext::~AIOContext() {
  int error_code = io_destroy(aio_ctx);
  if (error_code) {
    std::cerr << "WARNING: failed to destroy the AIO context" << std::endl;
  }
}

void AIOContext::queueWrite(int fd, uint8_t* data, std::size_t size,
                            std::size_t offset) {
  iocb* iocbp = new iocb;
  io_prep_pwrite(iocbp, fd, data, size, offset);
  queued_requests.push_back(iocbp);
}

void AIOContext::queueRead(int fd, uint8_t* data, std::size_t size,
                           std::size_t offset) {
  iocb* iocbp = new iocb;
  io_prep_pread(iocbp, fd, data, size, offset);
  queued_requests.push_back(iocbp);
}

std::size_t AIOContext::submitRequests() {
  // Submit all queued requests or enough to fill AIO queue.
  std::size_t request_count =
      std::min(queued_requests.size(), remainingAIOQueueDepth());

  // Get all requests that will be submitted in correct format.
  std::vector<iocb*> requests;
  requests.reserve(request_count);
  for (auto it = queued_requests.begin(); it != queued_requests.end(); ++it) {
    requests.push_back(*it);
  }

  // Submit all requests.
  int error_code = io_submit(aio_ctx, request_count, requests.data());
  if (error_code < 0) {
    throw std::system_error(-error_code, std::system_category(),
                            "failed to submit requests to AIO");
  }
  assert(error_code == request_count);

  // Remove the requests from our internal queue.
  assert(queued_requests.size() >= request_count);
  queued_requests.erase(queued_requests.begin(),
                        std::next(queued_requests.begin(), request_count));
  inflight_requests += request_count;
}

std::vector<AIOContext::AioEvent> AIOContext::collectRequests(
    std::size_t min_requests, std::size_t max_requests) {
  if (min_requests > max_requests) {
    throw std::runtime_error("min requests was greater than max requests");
  }
  if (min_requests > inflight_requests) {
    throw std::runtime_error(
        "requested more requests than are currently inflight");
  }

  io_event raw_events[max_requests];
  int event_count =
      io_getevents(aio_ctx, min_requests, max_requests, raw_events, nullptr);
  if (event_count < 0) {
    throw std::system_error(-event_count, std::system_category(),
                            "failed to get events");
  }
  assert(event_count >= min_requests);
  assert(event_count <= max_requests);

  std::vector<AioEvent> events;
  events.reserve(event_count);
  for (int i = 0; i < event_count; ++i) {
    events.push_back(Event(raw_events[i]));
  }
  return events;
}

std::size_t AIOContext::remainingAIOQueueDepth() {
  return queue_depth - inflight_requests;
}

AIOContext::AioEvent::AioEvent(io_event _event) : event(_event) {
  switch (event.obj->aio_lio_opcode) {
    case IO_CMD_PREAD: {
      type = EventType::read;
      break;
    }
    case IO_CMD_PWRITE: {
      type = EventType::write;
      break;
    }
    default: {
      throw std::runtime_error("given request type is not supported");
    }
  }
}

AIOContext::AioEvent::~AioEvent() {}

uint8_t* AIOContext::AioEvent::getData() {
  return static_cast<uint8_t*>(event.data);
}

AIOContext::AioEvent::EventType AIOContext::AioEvent::getType() { return type; }

#include <fcntl.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <system_error>
#include <thread>

// https://github.com/AKdroid/AIO-Test
int AioMain() {
  AIOContext aio_ctx(64);
  (void)aio_ctx;

  int fd = open("/dev/fake-dev0", O_DIRECT | O_RDWR | O_CLOEXEC);
  if (fd == -1) {
    throw std::system_error(errno, std::system_category(),
                            "failed to open test block device");
  }

  uint8_t* data = new uint8_t[4096];
  std::memset(data, 3, 4096);

  for (int i = 0; i < 10; ++i) {
    aio_ctx.queueWrite(fd, data, 4096, 0);
  }
  aio_ctx.submitRequests();

  auto events = aio_ctx.collectRequests(10, 10);
  std::cerr << events.size() << std::endl;
  for (auto event : events) {
    delete event.getData();
  }

  return 0;
}
#endif