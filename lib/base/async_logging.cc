#include "async_logging.h"
#include <assert.h>
#include <stdio.h>

using namespace MSF;

template <class BufferType, class Guard>
LoggingBase<BufferType, Guard>::LoggingBase(const std::string& basename,
                                            size_t roll_size,
                                            size_t partitions_,
                                            size_t flushInterval,
                                            const ThreadFunc& func)
    : running_(false),
      flush_interval_(flushInterval),
      basename_(basename),
      rollSize_(roll_size),
      partitions_(partitions_),
      thread_(func, "Logging"),
      latch_(1),
      mutex_() {
  int i = 0;
  buffers_.reserve(16);
  buffer_per_thread_.resize(partitions_);
  for (auto& currentBuffer : buffer_per_thread_) {
    BufferPtr buffer(new Buffer);
    buffer->bzero();
    currentBuffer = (std::move(buffer));
    // 初始化emptyBuffers_
    if (i++ < 6) {
      BufferPtr buffer2(new Buffer);
      emptyBuffers_.push_back(std::move(buffer2));
    }
  }
}

template <class BufferType, class Guard>
void LoggingBase<BufferType, Guard>::Start() {
  running_ = true;
  thread_.start();
  latch_.wait();
}

template <class BufferType, class Guard>
void LoggingBase<BufferType, Guard>::Stop() {
  running_ = false;
  cond_.notify_one();
  thread_.join();
}

template <class BufferType, class Guard>
size_t LoggingBase<BufferType, Guard>::GetIndex() {
  if (__cached_index < 0) {
    __cached_index = roundRobin_.fetch_add(1);
  }
  return __cached_index;
}

// index1 是 0 1 2 .. 7 0 1 2 ... 7
template <class BufferType, class Guard>
size_t LoggingBase<BufferType, Guard>::GetIndex1() {
  // 使用roundRoubin的策略进行获取
  return GetIndex() % partitions_;
}

// index2是 [0 0 0 .. 0 ] [1 .. 1 ]... [2 ... 2]
template <class BufferType, class Guard>
size_t LoggingBase<BufferType, Guard>::GetIndex2() {
  return (GetIndex() / partitions_) % partitions_;
}

template <class BufferType, class Guard>
void LoggingBase<BufferType, Guard>::SortBuffers(BufferVector& buffersToWrite) {
  // 将Buffer排序,避免pageFault
  size_t threshold = partitions_ * 2;
  std::sort(
      buffersToWrite.begin(), buffersToWrite.end(),
      [](BufferPtr& b1, BufferPtr& b2) { return b1->length() > b2->length(); });
  if (buffersToWrite.size() > threshold) {
    buffersToWrite.resize(threshold);
  }
}

template <class BufferType, class Guard>
void LoggingBase<BufferType, Guard>::FlushBuffers() {
  // 将buffer_per_thread里面的东西写入到buffers里面
  for (size_t i = 0; i < partitions_; ++i) {
    std::lock_guard<std::mutex> lock(mutex_);
    // const Guard& thread_guard = CreateGuard(i);
    auto& currentBuffer = buffer_per_thread_[i];

    buffers_.emplace_back(std::move(currentBuffer));
    if (!emptyBuffers_.empty()) {
      currentBuffer = std::move(emptyBuffers_.back());
      emptyBuffers_.pop_back();
    } else {
      // 这个地方出现的频率并不低,说明写入的频率比较高
      printf("new buffer when timeout \n");
      currentBuffer.reset(new Buffer);
    }
  }
}
template <class BufferType, class Guard>
void LoggingBase<BufferType, Guard>::RefillEmpty(BufferVector& buffersToWrite) {
  while (!buffersToWrite.empty()) {
    // assert(!buffersToWrite.empty());
    auto& buffer = buffersToWrite.back();
    buffer->reset();
    emptyBuffers_.push_back(std::move(buffer));
    buffersToWrite.pop_back();
  }
  buffersToWrite.clear();
}

template <class BufferType, class Guard>
void LoggingBase<BufferType, Guard>::ThreadLoop() {
  assert(running_);
  LogFile output(basename_, rollSize_ * 4, false);
  BufferVector buffersToWrite;
  buffersToWrite.reserve(16);
  // 为什么在这里才latch_?,防止刚启动就关闭的情况出现时候,running_来不及执行
  latch_.countDown();
  bool timeout = false;

  while (running_) {
    assert(buffersToWrite.empty());
    // 因为需要访问buffers,所以使用notify锁,这个锁的范围不能覆盖到下面的循环,否则会导致死锁(加锁顺序问题)
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (buffers_.empty()) {
        const int64_t kNanoSecondsPerSecond = 1000000000;
        int64_t nanoseconds =
            static_cast<int64_t>(flush_interval_ * kNanoSecondsPerSecond);
        cond_.wait_for(lock, std::chrono::nanoseconds(nanoseconds));
        // timeout?
      }
      if (timeout) {
        FlushBuffers();
      }
      buffersToWrite.swap(buffers_);
    }

    if (!buffersToWrite.empty()) {
      WriteBuffersToFile(buffersToWrite, output);
      SortBuffers(buffersToWrite);
      {
        std::lock_guard<std::mutex> lock2(mutex_);
        RefillEmpty(buffersToWrite);
      }
      output.flush();
    }
  }

  // 最后的结尾
  FlushBuffers();
  WriteBuffersToFile(buffersToWrite, output);
  WriteBuffersToFile(buffers_, output);
  output.flush();
}

AsyncLogging::AsyncLogging(const std::string filePath, off_t rollSize,
                           double flushInterval)
    : running_(false),
      file_path_(filePath),
      roll_size_(rollSize),
      flush_interval_(flushInterval),
      thread_(std::bind(&AsyncLogging::threadRoutine, this)),
      latch_(1),
      mutex_(),
      cond_(),
      current_buffer_(new Buffer),
      next_buffer_(new Buffer),
      buffers_() {}

AsyncLogging::~AsyncLogging() {
  if (running_) {
    stop();
  }
}

void AsyncLogging::append(const char* logline, int len) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (current_buffer_->avail() > len) {
    current_buffer_->append(logline, len);
  } else {
    buffers_.push_back(std::move(current_buffer_));

    if (next_buffer_) {
      current_buffer_ = std::move(next_buffer_);
    } else {
      current_buffer_.reset(new Buffer);
    }

    current_buffer_->append(logline, len);
    cond_.notify_one();
  }
}

void AsyncLogging::flush_notify() {
  std::unique_lock<std::mutex> lock(mutex_);
  cond_.notify_all();  // 保证日志及时输出
}

void AsyncLogging::threadRoutine() {
  assert(running_ == true);
  latch_.countDown();
  LogFile output(file_path_, roll_size_, false);
  BufferPtr newBuffer1(new Buffer);
  BufferPtr newBuffer2(new Buffer);
  newBuffer1->bzero();
  newBuffer2->bzero();
  BufferVector buffersToWrite;
  buffersToWrite.reserve(16);
  while (running_) {
    assert(newBuffer1 && newBuffer1->length() == 0);
    assert(newBuffer2 && newBuffer2->length() == 0);
    assert(buffersToWrite.empty());

    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (buffers_.empty())  // unusual usage!
      {
        const int64_t kNanoSecondsPerSecond = 1000000000;
        int64_t nanoseconds =
            static_cast<int64_t>(flush_interval_ * kNanoSecondsPerSecond);
        cond_.wait_for(lock, std::chrono::nanoseconds(nanoseconds));
      }
      buffers_.push_back(std::move(current_buffer_));
      current_buffer_ = std::move(newBuffer1);
      buffersToWrite.swap(buffers_);
      if (!next_buffer_) {
        next_buffer_ = std::move(newBuffer2);
      }
    }

    assert(!buffersToWrite.empty());

    if (buffersToWrite.size() > 25) {
      // char buf[256];
      // snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger
      // buffers\n",
      //          Timestamp::now().toFormattedString().c_str(),
      //          buffersToWrite.size()-2);
      // fputs(buf, stderr);
      // output.append(buf, static_cast<int>(strlen(buf)));
      // buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end());
    }

    for (const auto& buffer : buffersToWrite) {
      // FIXME: use unbuffered stdio FILE ? or use ::writev ?
      output.append(buffer->data(), buffer->length());
    }

    if (buffersToWrite.size() > 2) {
      // drop non-bzero-ed buffers, avoid trashing
      buffersToWrite.resize(2);
    }

    if (!newBuffer1) {
      assert(!buffersToWrite.empty());
      newBuffer1 = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      newBuffer1->reset();
    }

    if (!newBuffer2) {
      assert(!buffersToWrite.empty());
      newBuffer2 = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      newBuffer2->reset();
    }

    buffersToWrite.clear();
    output.flush();
  }
  output.flush();
}
