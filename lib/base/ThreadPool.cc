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
#include "ThreadPool.h"

#include <assert.h>
#include <butil/logging.h>

#include "Exception.h"
#include "Thread.h"

using namespace MSF;
using namespace MSF::CurrentThread;
using namespace MSF::THREAD;

namespace MSF {

ThreadPool::ThreadPool(const std::string& name)
    : _name(name),
      _running(false),
      _mutex(),
      _maxQueueSize(0),
      _notEmpty(),
      _notFull() {}

ThreadPool::~ThreadPool() {
  if (_running) {
    stop();
  }
}

void ThreadPool::start(int numThreads) {
  assert(_threads.empty());
  _threads.reserve(numThreads);
  _running = true;
  for (int i = 0; i < numThreads; ++i) {
    char id[32];
    snprintf(id, sizeof id, "%d", i + 1);
    _threads.emplace_back(
        new Thread(std::bind(&ThreadPool::runInThread, this), _name + id));
    _threads[i]->start();
    LOG(INFO) << "thread init success for:" << _name + id;
  }

  if (numThreads == 0 && _threadInitCallback) {
    _threadInitCallback();
  }
  LOG(INFO) << "all thread init completed";
}

void ThreadPool::stop() {
  /* 保证lock的保护范围在括号内激活所有的线程*/
  {
    std::lock_guard<std::mutex> lock(_mutex);
    _running = false;
  }
  _notEmpty.notify_all();
  for (auto& thr : _threads) {
    thr->join();
  }
}

size_t ThreadPool::queueSize() const {
  std::lock_guard<std::mutex> lock(_mutex);
  return _queue.size();
}

void ThreadPool::addTask(ThreadTask task) {
  if (_threads.empty()) {
    task();
  } else {
    std::unique_lock<std::mutex> lock(_mutex);
    while (isFull()) {
      _notFull.wait(lock);
    }
    if (!_running) {
      LOG(ERROR) << "thread pool not running";
      lock.unlock();
      return;
    }
    _queue.push_back(std::move(task));
    lock.unlock();
    _notEmpty.notify_one();
  }
}

ThreadPool::ThreadTask ThreadPool::getTask() {
  std::unique_lock<std::mutex> lock(_mutex);
  // always use a while-loop, due to spurious wakeup
  while (_queue.empty() && _running) {
    _notEmpty.wait(lock);
  }

  ThreadTask task = _queue.front();
  _queue.pop_front();
  lock.unlock();
  if (_maxQueueSize > 0) {
    _notFull.notify_one();
  }
  return task;
}

bool ThreadPool::isFull() const {
  return _maxQueueSize > 0 && _queue.size() >= _maxQueueSize;
}

void ThreadPool::runInThread() {
  try {
    if (_threadInitCallback) {
      _threadInitCallback();
    }
    while (_running) {
      ThreadTask task(getTask());
      if (task) {
        task();
      }
    }
  }
  catch (const MSF::Exception& ex) {
    LOG(FATAL) << "exception caught in ThreadPool " << _name.c_str()
               << ",reason: " << ex.what()
               << "stack trace: " << ex.stackTrace();
    abort();
  }
  catch (const std::exception& ex) {
    LOG(FATAL) << "exception caught in ThreadPool " << _name.c_str()
               << ",reason: " << ex.what();
    abort();
  }
  catch (...) {
    LOG(FATAL) << "unknown exception caught in ThreadPool " << _name.c_str();
    throw;  // rethrow
  }
}

}  // namespace MSF