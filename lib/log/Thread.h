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
#ifndef LIB_THREAD_H_
#define LIB_THREAD_H_

#include <thread>
#include <functional>
#include <memory>
#include <assert.h>

namespace MSF {

class Thread {
 public:
  typedef std::function<void()> ThreadFunc;

  explicit Thread(const ThreadFunc& threadRoutine);
  ~Thread();

  void start();
  void join();
  void detach();

  bool isStarted() { return m_isStarted; }
  bool isJoined() { return m_isJoined; }

  std::thread::id getThreadId() const {
    assert(m_isStarted);
    return p_thread->get_id();
  }

 private:
  Thread(const Thread&);
  const Thread& operator=(const Thread&);

  bool m_isStarted;
  bool m_isJoined;
  ThreadFunc m_threadRoutine;
  std::unique_ptr<std::thread> p_thread;
};

}  // namespace  MSF
#endif