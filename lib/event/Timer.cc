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
#include <base/Atomic.h>
#include <base/Thread.h>
#include <base/Time.h>
#include <base/Mem.h>
#include <event/Timer.h>


using namespace MSF::BASE;
using namespace MSF::TIME;
using namespace MSF::EVENT;
using namespace MSF::THREAD;

namespace MSF {
namespace EVENT {

HeapTimer::~HeapTimer()
{

}

void HeapTimer::initTimer()
{
    MSF_INFO << "Max timer num: " << _maxTimerNum;
  
}

void HeapTimer::addTimer(uint32_t id, uint32_t flag, uint32_t interval, void *arg, TimerCb cb)
{
    if (timer_queue.size() > _maxTimerNum) {
        MSF_ERROR << "up to max event num: " << _maxTimerNum;
        return;
    }
    struct TimerItem event(id, flag, interval, arg, cb);

    struct timeval now;
    gettimeofday(&now, nullptr);

    msf_memory_barrier();

    event.interval.tv_sec = interval / 1000;
    event.interval.tv_usec = (interval % 1000) * 1000;
    MSF_TIMERADD(&now, &(event.interval), &(event.expired));

    _queue.push(event);
}

void HeapTimer::removeTimer(uint32_t id)
{

}

void HeapTimer::timerHandler()
{
    struct timeval now;
    int ret = 0;

    while (!_queue.empty())
    {
        auto largest = _queue.top();

        gettimeofday(&now, nullptr);
        if (MSF_TIMERCMP(&now, &largest.expired, < )) {
            // MSF_ERROR << "Timer time is invalid";
             break;
        }
        _queue.pop();

        if (unlikely(largest.cb == nullptr)) {
            MSF_ERROR << "Timer cb is invalid";
            return;
        }
        
        ret = largest.cb(largest.id, largest.arg);
        //kill timer 
        if (ret == TIMER_ONESHOT) {
            largest.flag = TIMER_LIMITED;
            largest.exe_num = 1;
            MSF_INFO << "Timer cb ret is zero";
        }

        if (largest.flag == TIMER_PERSIST
        || (largest.flag == TIMER_LIMITED && --largest.exe_num > 0)) {
            MSF_TIMERADD(&largest.expired, &largest.interval, &largest.expired);
            _queue.push(largest);
            // MSF_INFO << "Timer add to list again, size:" << timer_queue.size();
        } else {
            // free(ev);
            MSF_INFO << "timer free it";
        }
    }
}
void HeapTimer::timerThread()
{
    MSF_INFO << "Timer thread id: " << std::this_thread::get_id();

    bool error = false;
    struct timeval now;
    struct timeval tv;
    struct timeval *tvp = NULL;
    long long ms = 0;
    uint64_t uIdleWait = 1;
    
    MSF_INFO << "Timer Thread started";
    while (!_quit) {

        if (unlikely(_queue.empty())) {
            if (++uIdleWait == 2000) {
                uIdleWait = 0;
            }
            tvp = NULL;
        } else {
            auto largest = _queue.top();

            gettimeofday(&now, nullptr);

            tvp = &tv;
            /* How many milliseconds we need to wait for the next
            * time event to fire? */
            ms = (largest.expired.tv_sec - now.tv_sec) * 1000 +
                    (largest.expired.tv_usec - now.tv_usec)/1000;

            if (ms > 0) {
                tvp->tv_sec = ms / 1000;
                tvp->tv_usec = (ms % 1000) * 1000;
            } else {
                tvp->tv_sec = 0;
                tvp->tv_usec = 0;
            }

            gettimeofday(&now, nullptr);

            tv.tv_sec = largest.expired.tv_sec - now.tv_sec;;
            tv.tv_usec = largest.expired.tv_usec - now.tv_usec;
            if ( tv.tv_usec < 0 ) {
                tv.tv_usec += 1000000;
                tv.tv_sec--;
                // MSF_INFO << "tv.tv_usec < 0.";
            }
            tvp = &tv;
        }

        if (tvp == nullptr) {
            // error = _ep->dispatchTimerEvent(uIdleWait);
        } else {
            // MSF_INFO << "timer_wait:%ld." << tvp->tv_sec*1000 + tvp->tv_usec/1000;//ms
            // error = _ep->dispatchTimerEvent(tvp->tv_sec*1000 + tvp->tv_usec/1000);
        }
        if (unlikely(!error)) {
            MSF_ERROR << "timer epoll wait error";
            break;
        }
        timerHandler();
    }
}

void HeapTimer::startTimer()
{
    std::thread timer_thread([this](){this->timerThread();});
    //https://www.cnblogs.com/little-ant/p/3312841.html
    timer_thread.detach();

    // std::this_thread::get_id()
    // std::thread::id timer_id = timer_thread.get_id();
}


void HeapTimer::stopTimer()
{

}
void HeapTimer::updateTime()
{

}

uint32_t HeapTimer::getTimerNumber()
{
    return timer_queue.size();
}






FdTimer::~FdTimer()
{

}
void FdTimer::initTimer()
{

}
void FdTimer::addTimer(uint32_t id, uint32_t flag, uint32_t interval, void *arg, TimerCb cb)
{

}
void FdTimer::removeTimer(uint32_t id)
{

}
void FdTimer::stopTimer()
{

}
void FdTimer::startTimer()
{
    #if 0
    // 创建timer fd
    int fd = timerfd_create(CLOCK_REALTIME, 0);
    if (fd < 0) {
        // _LOG_LAST_ERROR("timerfd_create failed(%s)", strerror(errno));
        return ;
    }

    //O_NONBLOCK

    // 设置超时时间
    struct itimerspec timeout;
    timeout.it_value.tv_sec     = timeout_ms / 1000;
    timeout.it_value.tv_nsec    = (timeout_ms % 1000) * 1000 * 1000;
    timeout.it_interval.tv_sec  = timeout.it_value.tv_sec;
    timeout.it_interval.tv_nsec = timeout.it_value.tv_nsec;

    if (timerfd_settime(fd, 0, &timeout, NULL) < 0) {
        close(fd);
        // _LOG_LAST_ERROR("timerfd_settime %d failed(%s)", fd, strerror(errno));
        return kSYSTEM_ERROR;
    }
    #endif
}
uint32_t FdTimer::getTimerNumber()
{
    return 0;
}

void FdTimer::timerHandler()
{

}
void FdTimer::timerThread()
{


}

void FdTimer::updateTime()
{
    // const auto now = std::chrono::steady_clock::now();
    const auto now = CurrentMilliTime();
    // if (xx > now )
}

}
}