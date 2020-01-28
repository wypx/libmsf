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
#include <base/CountDownLatch.h>

namespace MSF {
namespace BASE {

CountDownLatch::CountDownLatch(int count)
  : _count(count)
{
}

	
void CountDownLatch::wait()
{
  std::unique_lock <std::mutex> lock(_mutex);

  /**
   * 当前线程被阻塞 Unlock mu and wait to be notified 
   * https://www.jianshu.com/p/c1dfa1d40f53
   * https://zh.cppreference.com/w/cpp/thread/condition_variable
   * //https://blog.csdn.net/business122/article/details/80881925
   * */
  _condition.wait(lock, [this](){ return (_count == 0);} );
  
  lock.unlock();
}
 
void CountDownLatch::countDown()
{
  {
    std::lock_guard<std::mutex> lock(_mutex);
    --_count;
  }

  if (_count == 0)
  {
    _condition.notify_all();
  }
}
 
int CountDownLatch::getCount() const
{
  // std::lock_guard<std::mutex> lock(_mutex);
  return _count;
}

}
}