#include "AsyncLogging.h"
#include "LogFile.h"
#include <assert.h>
#include <stdio.h>

using namespace MSF;

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
  if (running_)
  {
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
  cond_.notify_all(); // 保证日志及时输出
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
  while (running_)
  {
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
      if (!next_buffer_)
      {
        next_buffer_ = std::move(newBuffer2);
      }
    }

    assert(!buffersToWrite.empty());

    if (buffersToWrite.size() > 25)
    {
      // char buf[256];
      // snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
      //          Timestamp::now().toFormattedString().c_str(),
      //          buffersToWrite.size()-2);
      // fputs(buf, stderr);
      // output.append(buf, static_cast<int>(strlen(buf)));
      // buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end());
    }

    for (const auto& buffer : buffersToWrite)
    {
      // FIXME: use unbuffered stdio FILE ? or use ::writev ?
      output.append(buffer->data(), buffer->length());
    }

    if (buffersToWrite.size() > 2)
    {
      // drop non-bzero-ed buffers, avoid trashing
      buffersToWrite.resize(2);
    }

    if (!newBuffer1)
    {
      assert(!buffersToWrite.empty());
      newBuffer1 = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      newBuffer1->reset();
    }

    if (!newBuffer2)
    {
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
