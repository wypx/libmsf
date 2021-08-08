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

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>

#include "latch.h"
#include "log_file.h"
#include "log_stream.h"
#include "spin_lock.h"
#include "thread.h"

using namespace MSF;

namespace MSF {

static const uint32_t kRollSize = 50 * 1024 * 1024;

__thread pid_t __cached_index = -1;

template <class BufferType, class Guard>
class LoggingBase {
 public:
  using Buffer = BufferType;
  using BufferVector = std::vector<std::unique_ptr<Buffer>>;
  using BufferPtr = std::unique_ptr<Buffer>;
  using StateVec = std::vector<std::atomic_uint32_t>;

  // virtual Guard CreateGuard(size_t i) = 0;
  virtual void WriteBuffersToFile(BufferVector& buffers, LogFile& output) = 0;

  LoggingBase(const std::string& basename, size_t roll_size, size_t partitions,
              size_t flushInterval, const ThreadCallback& func);

  void Start();
  void Stop();

  size_t GetIndex();
  size_t GetIndex1();
  size_t GetIndex2();
  void SortBuffers(BufferVector& buffersToWrite);
  void FlushBuffers();
  void RefillEmpty(BufferVector& buffersToWrite);
  void ThreadLoop();

 public:
  bool running_ = false;
  const int flush_interval_;
  std::string basename_;
  size_t rollSize_;
  size_t partitions_;
  std::atomic_uint32_t roundRobin_;
  std::unique_ptr<Thread> thread_;
  CountDownLatch latch_;
  std::mutex mutex_;
  std::condition_variable cond_;

  BufferVector buffers_;
  BufferVector emptyBuffers_;
  BufferVector buffer_per_thread_;
};

class AsyncLoggingDoubleBufferingShards
    : public LoggingBase<FixedBuffer<kLargeBuffer>, SpinLock> {
 public:
  AsyncLoggingDoubleBufferingShards(const std::string& basename,
                                    size_t rollSize, int flushInterval = 1)
      : LoggingBase(
            basename, rollSize, 8, flushInterval,
            std::bind(&AsyncLoggingDoubleBufferingShards::ThreadLoop, this)),
        mutexs_(partitions_) {}

  void append(const char* logline, int len) {
    // 随机选择一个mutex进行锁
    size_t index = GetIndex1();
    // std::lock_guard<std::mutex> lock(mutexs_[index]);
    BufferPtr& currentBuffer = buffer_per_thread_[index];

    if (currentBuffer->avail() > len) {
      currentBuffer->append(logline, len);
    } else {
      std::lock_guard<std::mutex> lock(mutex_);
      buffers_.push_back(std::move(currentBuffer));
      if (!emptyBuffers_.empty()) {
        currentBuffer = std::move(emptyBuffers_.back());
        emptyBuffers_.pop_back();
      } else {
        printf("new Buffer\n");
        currentBuffer.reset(new Buffer);  // Rarely happens
      }
      currentBuffer->append(logline, len);
      cond_.notify_one();
    }
  }

 private:
  // std::lock_guard CreateGuard(size_t i) override {
  //   std::lock_guard guard(mutexs_[i]);
  //   return guard;
  // }

  void WriteBuffersToFile(BufferVector& buffers, LogFile& output) override {
    for (auto& buffer : buffers) {
      output.append(buffer->data(), buffer->length());
    }
  }

 private:
  std::vector<SpinLock> mutexs_;
};

/***************CirCularBuffer版本**************************/
class States {
 public:
  using State = std::atomic_uint32_t;
  using StateVec = std::vector<State>;

  States(size_t dimension1, size_t dimension2)
      : dimension1(dimension1), dimension2(dimension2), states(dimension1) {
    for (StateVec& stateArr : states) {
      stateArr = StateVec(dimension2);
    }
  }

  State& get(size_t i, size_t j) { return states[i][j]; }

  StateVec& getVec(size_t i) { return states[i]; }

  size_t dimension1;
  size_t dimension2;
  std::vector<StateVec> states;
};

using CircularBuffer = CircularBufferTemplate<kLargeBuffer>;
class AsyncLoggingDoubleBuffering
    : public LoggingBase<CircularBuffer, WriteBarrier> {
 public:
  AsyncLoggingDoubleBuffering(const std::string& basename, size_t rollSize,
                              int flushInterval = 3, size_t partitions = 8)
      : LoggingBase(basename, rollSize, partitions, flushInterval,
                    std::bind(&AsyncLoggingDoubleBuffering::ThreadLoop, this)),
        states(LoggingBase::partitions_, LoggingBase::partitions_) {}

  void append(const char* logline, int len) {
    bool full;
    size_t remain = 0;
    auto index1 = GetIndex1();
    auto index2 = GetIndex2();
    BufferPtr& currentBuffer = buffer_per_thread_[index1];

    {
      ReadBarrier rb(states.get(index1, index2));
      full = !currentBuffer->append(logline, len, remain);
    }

    // 错开
    if (full || remain <= 8 * 1024 * (index1 + 1)) {
      std::lock_guard<std::mutex> lock(mutex_);
      WriteBarrier wb(states.getVec(index1));

      buffers_.emplace_back(std::move(currentBuffer));
      if (!emptyBuffers_.empty()) {
        currentBuffer = std::move(emptyBuffers_.back());
        emptyBuffers_.pop_back();
      } else {
        printf("new buffer\n");
        currentBuffer.reset(new Buffer);
      }

      currentBuffer->append(logline, len, remain);
      cond_.notify_one();
    }
  }

 private:
  void WriteBuffersToFile(BufferVector& buffers, LogFile& output) override {
    for (auto& buffer : buffers) {
      WriteCircularToFile(buffer, output);
    }
  }

  static void WriteCircularToFile(BufferPtr& buffer, LogFile& output) {
    // 将一个circularBuffer写入到output里面
    size_t len = FETCH_SIZE;
    char* data = nullptr;
    while ((data = buffer->peek(len)) != nullptr) {
      output.append(data, len);
      buffer->pop(len);
      len = FETCH_SIZE;
    }

    // 处理最后不足 FETCH_SIZE的部分
    if (len > 0 && len != FETCH_SIZE) {
      data = buffer->peek(len);
      assert(data);
      output.append(data, len);
      buffer->pop(len);
    }
  }

  // WriteBarrier CreateGuard(size_t i) override {
  //   WriteBarrier guard(states.getVec(i));
  //   return guard;
  // }

 private:
  States states;
  static constexpr size_t FETCH_SIZE =
      1024 * 1024 * 1;  //每次写1M的数据尝试获取
};

class AsyncLogging : noncopyable {
 public:
  AsyncLogging(const std::string filePath, off_t rollSize = kRollSize,
               double flushInterval = 3.0);
  ~AsyncLogging();

  void start() {
    running_ = true;
    thread_->Start();
    latch_.Wait();
  }

  void stop() {
    running_ = false;
    cond_.notify_one();
    thread_->Join();
  }

  void append(const char* logline, int len);
  void flush_notify();

 private:
  void AsyncLoggingLoop();

  typedef FixedBuffer<kLargeBuffer> Buffer;
  typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
  typedef BufferVector::value_type BufferPtr;

  bool running_;
  std::string file_path_;
  off_t roll_size_;
  const double flush_interval_;
  std::unique_ptr<Thread> thread_;
  CountDownLatch latch_;
  std::mutex mutex_;
  std::condition_variable cond_;

  BufferPtr current_buffer_;
  BufferPtr next_buffer_;
  BufferVector buffers_;
};

}  // namespace  MSF

#endif
