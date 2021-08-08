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
#include "flock.h"

#include <base/logging.h>
#include <fcntl.h>
#include <sys/file.h>

namespace MSF {

/*Linux/Unix文件系统中，有一种排它锁：WRLCK，
  只允许一次加锁成功，而且当进程无论主动退出还是被动退出,
  都会由操作系统释放。
  这种锁作用于pid文件上，特别适合于防止启动进程的多于一个副本
  防止进程启动多个副本。只有获得pid文件(固定路径固定文件名)
  写入权限(F_WRLCK)的进程才能正常启动并把自身的PID写入该文件中。
  其它同一个程序的多余进程则自动退出
  flock，建议性锁，不具备强制性.
  一个进程使用flock将文件锁住，另一个进程可以直接操作正在被锁的文件，
  修改文件中的数据，原因在于flock只是用于检测文件是否被加锁，
  针对文件已经被加锁，另一个进程写入数据的情况，
  内核不会阻止这个进程的写入操作，也就是建议性锁的内核处理策略
  flock主要三种操作类型：
  LOCK_SH，共享锁，多个进程可以使用同一把锁，常被用作读共享锁；
  LOCK_EX，排他锁，同时只允许一个进程使用，常被用作写锁；
  LOCK_UN，释放锁
  进程使用flock尝试锁文件时，如果文件已经被其他进程锁住，
  进程会被阻塞直到锁被释放掉，或者在调用flock的时候，
  采用LOCK_NB参数，在尝试锁住该文件的时候，发现已经被其他服务锁住，
  会返回错误，errno错误码为EWOULDBLOCK。
  即提供两种工作模式：阻塞与非阻塞类型
  flock锁的释放非常具有特色，即可调用LOCK_UN参数来释放文件锁，
  也可以通过关闭fd的方式来释放文件锁（flock的第一个参数是fd）,
  意味着flock会随着进程的关闭而被自动释放掉
  flock其中的一个使用场景为：检测进程是否已经存在
  在多线程开发中，互斥锁可以用于对临界资源的保护，防止数据的不一致，
  这是最为普遍的使用方法。那在多进程中如何处理文件之间的同步呢
  flock()的限制
  flock()放置的锁有如下限制
  只能对整个文件进行加锁。这种粗粒度的加锁会限制协作进程间的并发。
  假如存在多个进程，其中各个进程都想同时访问同一个文件的不同部分。
  通过flock()只能放置劝告式锁。
  很多NFS实现不识别flock()放置的锁。
  注释：在默认情况下，文件锁是劝告式的，这表示一个进程可以简单地忽略
  另一个进程在文件上放置的锁。要使得劝告式加锁模型能够正常工作，
  所有访问文件的进程都必须要配合，即在执行文件IO之前先放置一把锁。
  */

// https://blog.csdn.net/u013230767/article/details/77847075
// http://blog.chinaunix.net/uid-28541347-id-5678998.html
// https://www.cnblogs.com/starrysky77/p/9974130.html
PosixFileLock::~PosixFileLock() {
  if (fd_ > 0) {
    ::close(fd_);
    fd_ = -1;
  }
}

bool PosixFileLock::lock(bool create) {
  int32_t flag = 0;
  if (create) {
    flag = O_RDWR | O_CREAT | O_CLOEXEC | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  } else {
    flag = O_RDWR | O_CLOEXEC | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  }
  fd_ = ::open(file_name_.c_str(), flag, 0644);
  if (fd_ < 0) {
    LOG(ERROR) << "failed to open " << file_name_ << ", errno:" << errno;
    return false;
  }

  struct flock fl;
  memset(&fl, 0, sizeof(fl));
  fl.l_type = F_WRLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start = 0;
  fl.l_len = 0;
  if (::fcntl(fd_, F_SETLK, &fl) < 0) {
    if (errno == EAGAIN || errno == EACCES || errno == EWOULDBLOCK) {
      LOG(ERROR) << "failed to lock " << file_name_ << ", errno: " << errno
                 << ", maybe another process locked it";
    } else {
      LOG(ERROR) << "failed to lock " << file_name_ << ", errno: " << errno;
    }
    ::close(fd_);
    fd_ = -1;
    return false;
  }
  return true;
}

bool PosixFileLock::LockAll(bool create) {
  int32_t flag = 0;
  if (create) {
    flag = O_RDWR | O_CREAT | O_CLOEXEC | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  } else {
    flag = O_RDWR | O_CLOEXEC | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  }
  fd_ = ::open(file_name_.c_str(), flag, 0644);
  if (fd_ < 0) {
    LOG(ERROR) << "failed to open " << file_name_ << ", errno:" << errno;
    return false;
  }

  if (::flock(fd_, LOCK_EX | LOCK_NB) < 0) {
    if (errno == EAGAIN || errno == EACCES || errno == EWOULDBLOCK) {
      LOG(ERROR) << "failed to lock " << file_name_ << ", errno: " << errno
                 << ", maybe another process locked it";
    } else {
      LOG(ERROR) << "failed to lock " << file_name_ << ", errno: " << errno;
    }
    ::close(fd_);
    fd_ = -1;
    return false;
  }
  return true;
}

bool PosixFileLock::LockRead(uint32_t start, uint32_t whence, uint32_t len) {
  struct flock lock;
  lock.l_type = F_RDLCK;
  lock.l_start = start;
  lock.l_whence = whence;  // SEEK_CUR,SEEK_SET,SEEK_END
  lock.l_len = len;
  lock.l_pid = ::getpid();
  return (::fcntl(fd_, F_SETLKW, &lock) == 0);
}

/* 该将会阻塞进程的执行,使用时需要非常谨慎,
 * 它可能会导致worker进程宁可睡眠也不处理其他正常请
 */
bool PosixFileLock::LockWrite() {
  struct flock fl;
  ::memset(&fl, 0, sizeof(struct flock));
  fl.l_type = F_WRLCK;
  fl.l_whence = SEEK_SET;
  return (::fcntl(fd_, F_SETLKW, &fl) == 0);
}

bool PosixFileLock::TryLockRead(uint32_t start, uint32_t whence, uint32_t len) {
  struct flock lock;
  lock.l_type = F_RDLCK;
  lock.l_start = start;
  lock.l_whence = whence;  // SEEK_CUR,SEEK_SET,SEEK_END
  lock.l_len = len;
  lock.l_pid = ::getpid();
  /* 阻塞方式加锁 */
  return (::fcntl(fd_, F_SETLK, &lock) == 0);
}

/* flock锁的释放非常具有特色:
 * 即可调用LOCK_UN参数来释放文件锁
 * 也可以通过关闭fd的方式来释放文件锁
 * 意味着flock会随着进程的关闭而被自动释放掉
 */
bool PosixFileLock::TryLockWrite() {
  struct flock fl;
  /* 这个文件锁并不用于锁文件中的内容,填充为0 */
  ::memset(&fl, 0, sizeof(struct flock));
  fl.l_type = F_WRLCK; /*F_WRLCK意味着不会导致进程睡眠*/
  fl.l_whence = SEEK_SET;

  /* 获取互斥锁成功时会返回0,否则返回的其实是errno错误码,
   * 而这个错误码为EAGAIN或者EACCESS时表示当前没有拿到互斥锁,
   * 否则可以认为fcntl执行错误
   */
  return (::fcntl(fd_, F_SETLK, &fl) == 0);
}

bool PosixFileLock::unlock() {
  if (fd_ < 0) {
    LOG(ERROR) << "file name: " << file_name_ << " has not been opened";
    return false;
  }
  struct flock fl;
  ::memset(&fl, 0, sizeof(fl));
  fl.l_type = F_UNLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start = 0;
  fl.l_len = 0;  // Lock/unlock entire file
  if (::fcntl(fd_, F_SETLK, &fl) < 0) {
    LOG(ERROR) << "failed to unlock " << file_name_ << ", errno: " << errno;
    ::close(fd_);
    fd_ = -1;
    return false;
  }
  return true;
}
}  // namespace MSF