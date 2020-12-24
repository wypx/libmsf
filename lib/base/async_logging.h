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
#ifndef LIB_ASYNC_LOGGING_H_
#define LIB_ASYNC_LOGGING_H_

#include <memory>
#include <mutex>
#include <string>
#include <condition_variable>

#include "latch.h"
#include "thread.h"
#include "log_stream.h"

using namespace MSF;
using namespace MSF::THREAD;

namespace MSF {

static const uint32_t kRollSize = 50 * 1024 * 1024;

class AsyncLogging : Noncopyable {
 public:
  AsyncLogging(const std::string filePath, off_t rollSize = kRollSize,
               double flushInterval = 3.0);
  ~AsyncLogging();

  void start() {
    running_ = true;
    thread_.start();
    latch_.wait();
  }

  void stop() {
    running_ = false;
    cond_.notify_one();
    thread_.join();
  }

  void append(const char* logline, int len);
  void flush_notify();

 private:
  void threadRoutine();

  typedef FixedBuffer<kLargeBuffer> Buffer;
  typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
  typedef BufferVector::value_type BufferPtr;

  bool running_;
  std::string file_path_;
  off_t roll_size_;
  const double flush_interval_;
  Thread thread_;
  CountDownLatch latch_;
  std::mutex mutex_;
  std::condition_variable cond_;

  BufferPtr current_buffer_;
  BufferPtr next_buffer_;
  BufferVector buffers_;
};

}  // namespace  MSF

#endif
