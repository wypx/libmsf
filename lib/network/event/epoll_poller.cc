
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
#include "epoll_poller.h"

#include <base/logging.h>

#include "event.h"
#include "gcc_attr.h"
#include "utils.h"

using namespace MSF;

namespace MSF {

/*
 * MSF_EVENT_DISABLED state is used to track whether EPOLLONESHOT
 * event should be added or modified, epoll_ctl(2):
 *
 * EPOLLONESHOT (since Linux 2.6.2)
 *     Sets the one-shot behavior for the associated file descriptor.
 *     This means that after an event is pulled out with epoll_wait(2)
 *     the associated file descriptor is internally disabled and no
 *     other events will be reported by the epoll interface.  The user
 *     must call epoll_ctl() with EPOLL_CTL_MOD to rearm the file
 *     descriptor with a new event mask.
 *
 * Notice:
 *  Although calling close() on a file descriptor will remove any epoll
 *  events that reference the descriptor, in this case the close() acquires
 *  the kernel global "epmutex" while epoll_ctl(EPOLL_CTL_DEL) does not
 *  acquire the "epmutex" since Linux 3.13 if the file descriptor presents
 *  only in one epoll set.  Thus removing events explicitly before closing
 *  eliminates possible lock contention.
 */
EPollPoller::EPollPoller(EventLoop* loop) : Poller(loop) {
  CreateEpollSocket();
}

EPollPoller::~EPollPoller() {
  if (poll_fd_ > 0) {
    ::close(poll_fd_);
    poll_fd_ = -1;
  }
}

/* On Linux kernels at least up to 2.6.24.4, epoll can't handle timeout
 * values bigger than (LONG_MAX - 999ULL)/HZ.  HZ in the wild can be
 * as big as 1000, and LONG_MAX can be as small as (1<<31)-1, so the
 * largest number of msec we can support here is 2147482.  Let's
 * round that down by 47 seconds.
 */
bool EPollPoller::CreateEpollSocket() {
  /* First, try the shiny new epoll_create1 interface, if we have it. */
  poll_fd_ = ::epoll_create1(EPOLL_CLOEXEC);
  if (poll_fd_ < 0 && (errno == ENOSYS || errno == EINVAL)) {
    /* Initialize the kernel queue using the old interface.
     * (The size field is ignored   since 2.6.8.) */
    poll_fd_ = ::epoll_create(1024);
    if (poll_fd_ < 0) {
      if (errno != ENOSYS) {
        LOG(FATAL) << "failed to create epoll fd: " << errno;
        return false;
      }
      return false;
    }
    SetCloseOnExec(poll_fd_);
  }
  LOG(TRACE) << "epoll create fd: " << poll_fd_;
  return true;
}

bool EPollPoller::AddEvent(const Event* ev) {
  struct epoll_event event;
  memset(&event, 0, sizeof event);
  event.events = ev->events();
  event.data.ptr = (void*)ev;

  if (::epoll_ctl(poll_fd_, EPOLL_CTL_ADD, ev->fd(), &event) < 0) {
    /* If an add operation fails with EEXIST,
     * either the operation was redundant (as with a
     * precautionary add), or we ran into a fun
     * kernel bug where using dup*() to duplicate the
     * same file into the same fd gives you the same epitem
     * rather than a fresh one.  For the second case,
     * we must retry with mod. */
    if (errno == EEXIST) {
      if (::epoll_ctl(poll_fd_, EPOLL_CTL_MOD, ev->fd(), &event) < 0) {
        LOG(ERROR) << "epoll mod on" << ev->fd()
                   << " retried as add; that failed too.";
        return true;
      } else {
        LOG(ERROR) << "epoll mod on" << ev->fd()
                   << " retried as add; succeeded.";
        return false;
      }
    }
  }
  LOG(TRACE) << "epoll add"
             << " on " << ev->fd() << " succeeded.";
  poll_events_.resize(poll_events_.size() + 1);
  return true;
}

bool EPollPoller::ModifyEvent(const Event* ev) {
  struct epoll_event event;
  memset(&event, 0, sizeof event);
  event.events = ev->events();
  event.data.ptr = (void*)ev;

  if (::epoll_ctl(poll_fd_, EPOLL_CTL_MOD, ev->fd(), &event) < 0) {
    LOG(TRACE) << "failed to mod epoll event";
    /* If a mod operation fails with ENOENT, the
     * fd was probably closed and re-opened.  We
     * should retry the operation as an add.
     */
    if (errno == ENOENT) {
      if (::epoll_ctl(poll_fd_, EPOLL_CTL_ADD, ev->fd(), &event) < 0) {
        LOG(ERROR) << "epoll mod"
                   << " on " << ev->fd() << " retried as add; that failed too.";
        return true;
      } else {
        LOG(ERROR) << "epoll mod"
                   << " on " << ev->fd() << " retried as add; succeeded.";
        return false;
      }
    }
  }
  LOG(TRACE) << "epoll mod"
             << " on " << ev->fd() << " succeeded.";
  return true;
}

bool EPollPoller::DeleteEvent(const Event* ev) {
  if (::epoll_ctl(poll_fd_, EPOLL_CTL_DEL, ev->fd(), NULL) < 0) {
    LOG(ERROR) << "Failed to del epoll event for fd: " << ev->fd() << ",errno"
               << errno;
    if (errno == ENOENT || errno == EBADF || errno == EPERM) {
      /* If a delete fails with one of these errors,
       * that's fine too: we closed the fd before we
       * got around to calling epoll_dispatch. */
      LOG(ERROR) << "epoll DEL on fd " << ev->fd() << "gave " << strerror(errno)
                 << "DEL was unnecessary.";
      return true;
    }
    return false;
  }
  LOG(TRACE) << "epoll Del"
             << " on " << ev->fd() << " succeeded.";
  return true;
}

// https://cloud.tencent.com/developer/ask/223634
int EPollPoller::Poll(int timeout_ms, EventList* active_events) {
  int num_events = 0;

  // https://www.cnblogs.com/likwo/archive/2012/12/20/2826988.html
  uint64_t sigmask = 0;
  if (has_signal_profile()) {
    ::sigemptyset(&sigset_);
    ::sigaddset(&sigset_, SIGPROF);
    sigmask |= 1 << (SIGPROF - 1);
  }

  if (sigmask != 0 && use_epoll_pwait_) {
    if (::pthread_sigmask(SIG_BLOCK, &sigset_, nullptr)) {
      LOG(ERROR) << "pthread sigmask sig block errno: " << errno;
      use_epoll_pwait_ = false;
    }
  } else {
    use_epoll_pwait_ = false;
  }

  if (use_epoll_pwait_) {
    num_events = ::epoll_pwait(poll_fd_, &*poll_events_.begin(),
                               static_cast<int>(poll_events_.size()),
                               timeout_ms, &sigset_);
    if (num_events == -1 && errno == ENOSYS) {
      use_epoll_pwait_ = false;
    }
  } else {
    num_events =
        ::epoll_wait(poll_fd_, &*poll_events_.begin(),
                     static_cast<int>(poll_events_.size()), timeout_ms);
  }

  if (use_epoll_pwait_) {
    if (::pthread_sigmask(SIG_UNBLOCK, &sigset_, nullptr) < 0) {
      use_epoll_pwait_ = false;
    }
  }
  if (::pthread_sigmask(SIG_UNBLOCK, &sigset_, NULL)) abort();
  // LOG(INFO) << "_activeevents_ size: " << active_events.size();
  // Timestamp now(Timestamp::now());
  if (num_events > 0) {
    // LOG(INFO) << num_events << " events happened";
    FillActiveEvents(num_events, active_events);
    // https://blog.csdn.net/xiaoc_fantasy/article/details/79570788
    if (implicit_cast<size_t>(num_events) == poll_events_.size()) {
      poll_events_.resize(poll_events_.size() * 2);
    }
  } else if (num_events == 0) {
    LOG(TRACE) << "timeout cause nothing happened";
  } else {
    // error happens, log uncommon ones
    if (errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN) {
      LOG(ERROR) << "EPollPoller::poll(): " << errno;
    }
    LOG(ERROR) << "EPollPoller::poll(): " << errno;
  }
  return 0;
}

void EPollPoller::FillActiveEvents(int num_events, EventList* active_events) {
  /*
   * Notice:
   * epoll event struct's "data" is a union,
   * one of data.fd and data.ptr is valid.
   * refer:
   * https://blog.csdn.net/sun_z_x/article/details/22581411
   */
  for (int i = 0; i < num_events; ++i) {
    Event* ev = static_cast<Event*>(poll_events_[i].data.ptr);
#ifndef NDEBUG
    int fd = ev->fd();
    auto it = events_.find(fd);
    assert(it != events_.end());
    assert(it->second == ev);
#endif
    ev->set_revents(poll_events_[i].events);
    active_events->push_back(ev);
  }
}

void EPollPoller::UpdateEvent(Event* ev) {
  const int index = ev->index();
  LOG(TRACE) << "fd = " << ev->fd() << " events = " << ev->events()
             << " index = " << index;

  Poller::AssertInLoopThread();

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
void EPollPoller::RemoveEvent(Event* ev) {
  Poller::AssertInLoopThread();
  int fd = ev->fd();
  if (unlikely(events_.find(fd) == events_.end())) {
    LOG(ERROR) << "not found in event map, fd: " << fd;
    return;
  }
  if (unlikely(events_[fd] != ev)) {
    LOG(ERROR) << "event: " << ev << " and fd: " << fd << " not match";
    return;
  }
  if (unlikely(ev->IsNoneEvent())) {
    LOG(ERROR) << "event: " << ev << " and fd: " << fd << " has none event";
    return;
  }
  int index = ev->index();
  assert(index == kAdded || index == kDeleted);
  LOG(TRACE) << "remove fd: " << fd;
  size_t n = events_.erase(fd);
  (void)n;
  assert(n == 1);

  if (index == kAdded) {
    DeleteEvent(ev);
  }
  ev->set_index(kNew);
}

const char* EPollPoller::OperationToString(int op) {
  switch (op) {
    case EPOLL_CTL_ADD:
      return "add";
    case EPOLL_CTL_DEL:
      return "DEL";
    case EPOLL_CTL_MOD:
      return "mod";
    default:
      assert(false && "ERROR op");
      return "Unknown Operation";
  }
}

}  // namespace MSF

// http://www.cnblogs.com/Anker/archive/2013/08/17/3263780.html
/* http://blog.csdn.net/yusiguyuan/article/details/15027821
EPOLLET:
将EPOLL设为边缘触发(Edge Triggered)模式,
这是相对于水平触发(Level Triggered)来说的
EPOLL事件有两种模型：
Edge Triggered(ET)
高速工作方式,错误率比较大,只支持no_block socket (非阻塞socket)
LevelTriggered(LT)
缺省工作方式，即默认的工作方式,支持blocksocket和no_blocksocket,错误率比较小.
EPOLLIN:
listen fd,有新连接请求,对端发送普通数据
EPOLLPRI:
有紧急的数据可读(这里应该表示有带外数据到来)
EPOLLERR:
表示对应的文件描述符发生错误
EPOLLHUP:
表示对应的文件描述符被挂断
对端正常关闭(程序里close(),shell下kill或ctr+c),触发EPOLLIN和EPOLLRDHUP,
但是不触发EPOLLERR 和EPOLLHUP.再man epoll_ctl看下后两个事件的说明,
这两个应该是本端（server端）出错才触发的.
EPOLLRDHUP:
这个好像有些系统检测不到,可以使用EPOLLIN,read返回0,删除掉事件,关闭close(fd)
EPOLLONESHOT:
只监听一次事件,当监听完这次事件之后,
如果还需要继续监听这个socket的话,
需要再次把这个socket加入到EPOLL队列里
对端异常断开连接(只测了拔网线),没触发任何事件。
epoll的优点：
1.支持一个进程打开大数目的socket描述符(FD)
select 最不能忍受的是一个进程所打开的FD是有一定限制的，
由FD_SETSIZE设置，默认值是2048。
对于那些需要支持的上万连接数目的IM服务器来说显然太少了。
这时候你一是可以选择修改这个宏然后重新编译内核，
不过资料也同时指出这样会带来网络效率的下降，
二是可以选择多进程的解决方案(传统的 Apache方案)，
不过虽然linux上面创建进程的代价比较小，但仍旧是不可忽视的，
加上进程间数据同步远比不上线程间同步的高效，所以也不是一种完美的方案。
不过 epoll则没有这个限制，它所支持的FD上限是最大可以打开文件的数目，
这个数字一般远大于2048,举个例子,在1GB内存的机器上大约是10万左右，
具体数目可以cat /proc/sys/fs/file-max察看,一般来说这个数目和系统内存关系很大。
2.IO效率不随FD数目增加而线性下降
传统的select/poll另一个致命弱点就是当你拥有一个很大的socket集合，
不过由于网络延时，任一时间只有部分的socket是"活跃"的，
但是select/poll每次调用都会线性扫描全部的集合，导致效率呈现线性下降。
但是epoll不存在这个问题，它只会对"活跃"的socket进行操作---
这是因为在内核实现中epoll是根据每个fd上面的callback函数实现的。
那么，只有"活跃"的socket才会主动的去调用 callback函数，
其他idle状态socket则不会，在这点上，epoll实现了一个"伪"AIO，
因为这时候推动力在os内核。在一些 benchmark中，
如果所有的socket基本上都是活跃的---比如一个高速LAN环境，
epoll并不比select/poll有什么效率，相反，如果过多使用epoll_ctl,
效率相比还有稍微的下降。但是一旦使用idle connections模拟WAN环境,
epoll的效率就远在select/poll之上了
3.使用mmap加速内核与用户空间的消息传递
这点实际上涉及到epoll的具体实现了。
无论是select,poll还是epoll都需要内核把FD消息通知给用户空间，
如何避免不必要的内存拷贝就很重要，在这点上，
epoll是通过内核于用户空间mmap同一块内存实现的。
而如果你想我一样从2.5内核就关注epoll的话，一定不会忘记手工 mmap这一步的。
4.内核微调
这一点其实不算epoll的优点了，而是整个linux平台的优点。
也许你可以怀疑linux平台，但是你无法回避linux平台赋予你微调内核的能力。
比如，内核TCP/IP协议栈使用内存池管理sk_buff结构，
那么可以在运行时期动态调整这个内存pool(skb_head_pool)的大小---
通过echoXXXX>/proc/sys/net/core/hot_list_length完成。
再比如listen函数的第2个参数(TCP完成3次握手的数据包队列长度)，
也可以根据你平台内存大小动态调整。
更甚至在一个数据包面数目巨大但同时每个数据包本身大小却很小的特殊系统上
尝试最新的NAPI网卡驱动架构。
*/