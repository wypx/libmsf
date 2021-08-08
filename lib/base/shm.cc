
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
#include "shm.h"

#include <base/logging.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>

#include "file.h"

using namespace MSF;

namespace MSF {

/* 共享内存
    共享内存是Linux下提供的最基本的进程间通信方法，
    它通过mmap或者shmget系统调用在内存中创建了一块连续的线性地址空间，
    而通过munmap或者shmdt系统调用可以释放这块内存。
    使用共享内存的好处是当多个进程使用同一块共享内存时，在任何一个进程修改了共享内存中的内容后，
    其他进程通过访问这段共享内存都能够得到修改后的内容。
    注意:
    虽然mmap可以以磁盘文件的方式映射共享内存，
    但在Nginx封装的共享内存操作方法中是没有使用到映射文件功能的.

    操作ngx_shm_t结构体的方法有以下两个：
    ngx_shm_alloc用于分配新的共享内存，
    ngx_shm_free用于释放已经存在的共享内存。
    在描述这两个方法前，先以mmap为例说朗
    Linux是怎样向应用程序提供共享内存的，如下所示。
    void *mmap (void *start, size_t length,  int prot,  int flags, int fd, off_t
   offset);
    mmap可以将磁盘文件映射到内存中,直接操作内存时Linux内核将负责同步内存和磁盘文件中的数据，
    fd就指向需要同步的磁盘文件,而offset则代表从文件的这个偏移量处开始共享,当然Nginx没有使用这一特性
    当flags参数中加入MAP
    ANON或者MAP—ANONYMOUS参数时表示不使用文件映射方式，这时fd和offset参数就没有意义，
    也不需要传递了,此时的mmap方法和ngx_shm_alloc的功能几乎完全相同。length参数就是将要在内存中开辟的线性地址空间大小,而prot参数则是操作这段共享内存的方式(如只读或者可读可写),start参数说明希望的共享内存起始映射地址，当然，通常都会把start设为NULL空指针

    先来看看如何使用mmap实现ngx_shm_alloc方法，代码如下。
    ngx_int_t ngx_shm_ alloc (ngx_shm_t *shm)
    {
        ／／开辟一块shm-
   >size大小且可以读／写的共享内存，内存首地址存放在map_addr_中
        shm->map_addr_=  (uchar *)mmap (NULL,  shm->size,
        PROT_READ l PROT_WRITE,
        MAP_ANONIMAP_SHARED,  -1,o);
    if (shm->map_addr_ == MAP_FAILED)
        return NGX ERROR;
    }
        return NGX OK;
        )
    这里不再介绍shmget方法申请共享内存的方式，它与上述代码相似。
    当不再使用共享内存时，需要调用munmap或者shmdt来释放共享内存，这里还是以与mmap配对的munmap为例来说明。
    其中，start参数指向共享内存的首地址，而length参数表示这段共享内存的长度。
    下面看看ngx_shm_free方法是怎样通过munmap来释放共享内存的。
    void  ngx_shm—free (ngx_shm_t★shm)
    {
        ／／使用ngx_shm_t中的map_addr_和size参数调用munmap释放共享内存即可
        if  (munmap( (void*)  shm->map_addr_,  shm- >size)  ==~1)  (
        ngx_LOG(ERROR) (NGX—LOG__  ALERT,  shm- >log,
            ngx_errno,  ”munmap(%p,  %uz)failed",  shm- >map_addr_,   shm-
   >size)j
        )
    )
    Nginx各进程间共享数据的主要方式就是使用共享内存（在使用共享内存时，Nginx -
    般是由master进程创建，在master进程fork出worker子进程后，所有的进程开始使用这
    块内存中的数据）。在开发Nginx模块时如果需要使用它，不妨用Nginx已经封装好的ngx_
    shm—alloc方法和ngx_shm_free方法，它们有3种实现:
    不映射文件使用mmap分配共享内存、
    以/dev/zero文件使用mmap映射共享内存、
    用shmget调用来分配共享内存((system-v标准))，
    对于Nginx的跨平台特性考虑得很周到。下面以一个统计HTTP框架连接状况的例子来说明共享内存的用法。
*/

ShmManager::ShmManager(const std::string &name, uint64_t size) {}

int ShmManager::ShmAllocByMapFile(const std::string &filename) {
  map_addr_ = (char *)::mmap(NULL, size, PROT_READ | PROT_WRITE,
                             MAP_ANON | MAP_SHARED, -1, 0);

  if (map_addr_ == MAP_FAILED) {
    return -1;
  }
  return 0;
}

bool ShmManager::InitializeSysv() {
#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
/* union semun is defined by including <sys/sem.h> */
#else
  /* according to X/OPEN we have to define it ourselves */
  union semun {
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT, IPC_SET */
    unsigned short *array; /* array for GETALL, SETALL */
    /* Linux specific part: */
    struct seminfo *__buf; /* buffer for IPC_INFO */
  };
#endif
  sem_id_ = ::semget(shm_key_, 2, IPC_CREAT | IPC_EXCL | 0666);
  if (sem_id_ < 0) {
    // 判断信号量是否已经存在
    if (errno != EEXIST) {
      LOG(ERROR) << "sem has exist error: " << errno;
      return false;
    }
    sem_id_ = ::semget(shm_key_, 2, 0666);
    if (sem_id_ < 0) {
      LOG(ERROR) << "shm get error: " << errno;
      return false;
    }
  } else {
    union semun sem_arg;
    // 包含两个信号量,第一个为写信号两，第二个为读信号两
    uint16_t array[2] = {0, 0};
    // 生成信号量集
    sem_arg.array = &array[0];
    // 将所有信号量的值设置为0
    if (::semctl(sem_id_, 0, SETALL, sem_arg) < 0) {
      LOG(ERROR) << "shm ctl error: " << errno;
      return false;
    }
  }
  return true;
}

bool ShmManager::InitializePosix() { return true; }

bool ShmManager::Initialize() {
  switch (alloc_method_) {
    case kAllocateSysvShmget: {
      return InitializeSysv();
    };
    case kAllocatePosixShmopen: {
      return InitializePosix();
    }
    default:
      break;
  }
  return true;
}

bool ShmManager::AllocateMappingFile() { return false; }

bool ShmManager::AllocateMappingMem() {
  // https://blog.csdn.net/callinglove/article/details/46710465
  // https://blog.csdn.net/qq_33611327/article/details/81738195
  map_addr_ = (char *)::mmap(NULL, size, PROT_READ | PROT_WRITE,
                             MAP_ANON | MAP_SHARED, -1, 0);

  // map_addr_ = (uint8_t *) mmap(NULL, size, PROT_READ | PROT_WRITE,
  //     MAP_PRIVATE | MAP_ANONYMOUS |
  //     MAP_POPULATE | MAP_HUGETLB, -1, 0);

  if (map_addr_ == MAP_FAILED) {
    LOG(ERROR) << "mapping mem failed, errno: " << errno;
    return false;
  }
  return true;
}

bool ShmManager::AllocateMappingDevZero() {
  int fd = ::open("/dev/zero", O_RDWR);
  if (fd == -1) {
    return false;
  }
  map_addr_ =
      (char *)::mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (map_addr_ == MAP_FAILED) {
    LOG(ERROR) << "mmap(/dev/zero, MAP_SHARED, " << size << "failed";
    return false;
  }
  if (::close(fd) < 0) {
    return false;
  }
  return (map_addr_ == MAP_FAILED) ? false : true;
}

bool ShmManager::AllocateShmget() {
  // https://blog.csdn.net/ypt523/article/details/79958188?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-1.channel_param&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-1.channel_param
  // https://blog.csdn.net/21cnbao/article/details/102994278?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-1.compare&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-1.compare
  // https://www.cnblogs.com/charlesblc/p/6261469.html
  // https://blog.csdn.net/u014304293/article/details/46386057
  // Linux共享内存使用常见陷阱与分析
  // https://www.cnblogs.coms/Marineking/p/3747217.html
  // IPC_EXCL
  sem_id_ = ::shmget(shm_key_, size, (SHM_R | SHM_W | IPC_CREAT));
  // int id = ::shmget(IPC_PRIVATE, size, (SHM_R | SHM_W | IPC_CREAT));
  if (sem_id_ == -1) {
    return false;
  }

  LOG(INFO) << "shmget id: " << sem_id_;

  map_addr_ = (char *)::shmat(sem_id_, NULL, 0);
  if (map_addr_ == (void *)-1) {
    LOG(ERROR) << "shmat mem failed: " << strerror(errno);
  }
  if (::shmctl(sem_id_, IPC_RMID, NULL) < 0) {
    LOG(ERROR) << "shmctl mem failed: " << strerror(errno);
  }
  return (map_addr_ == (void *)-1) ? false : true;
}

bool ShmManager::AllocateShmopen() {
#if defined(WIN32) || defined(WIN64)
  // Opens a named file mapping object.
  HANDLE h;
  if (mode == open) {
    h = ::OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, shm_name_.c_str());
  }
  // Creates or opens a named file mapping object for a specified file.
  else {
    h = ::CreateFileMapping(INVALID_HANDLE_VALUE, detail::get_sa(),
                            PAGE_READWRITE | SEC_COMMIT, 0,
                            static_cast<DWORD>(size), shm_name_.c_str());
    // If the object exists before the function call, the function returns a
    // handle to the existing object
    // (with its current size, not the specified size), and GetLastError returns
    // ERROR_ALREADY_EXISTS.
    if ((mode == create) && (::GetLastError() == ERROR_ALREADY_EXISTS)) {
      ::CloseHandle(h);
      h = NULL;
    }
  }

  LPVOID mem = ::MapViewOfFile(ii->h_, FILE_MAP_ALL_ACCESS, 0, 0, 0);
  if (mem == NULL) {
    LOG(ERROR) << "fail MapViewOfFile: " << static_cast<int>(::GetLastError());
    return nullptr;
  }
  MEMORY_BASIC_INFORMATION mem_info;
  if (::VirtualQuery(mem, &mem_info, sizeof(mem_info)) == 0) {
    LOG(ERROR) << "fail VirtualQuery: " << static_cast<int>(::GetLastError());
    return nullptr;
  }
  shm_size_ = static_cast<std::size_t>(mem_info.RegionSize);

  ::UnmapViewOfFile(static_cast<LPCVOID>(mem));
#else
  // https://www.cnblogs.com/charlesblc/p/6261469.html
  region_size_ = ::sysconf(_SC_PAGE_SIZE);
  // shm_fd_ =
  //     ::shm_open(shm_name_.c_str(), O_CREAT | O_TRUNC | O_RDWR,
  //                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
  // O_CREAT | O_EXCL
  // The check for the existence of the object,
  // and its creation if it does not exist, are performed atomically.
  // O_CREAT
  // Create the shared memory object if it does not exist.
  int fd = ::shm_open(shm_name_.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0666);
  if (fd < 0) {
    LOG(ERROR) << "posix shm open failed, errno: " << errno;
    return false;
  }
  int ret = ::ftruncate(fd, region_size_);
  if (ret != 0) {
    return false;
  }
  // https://bbs.csdn.net/topics/320060705

  // MAP_FIXED标志的疑惑
  // http://blog.chinaunix.net/uid-25538637-id-195878.html
  // https://www.it1352.com/359189.html
  // https://blog.csdn.net/rikeyone/article/details/85223796
  // http://www.360doc.com/content/19/0627/14/13328254_845181221.shtml
  posix_shm_addr_ =
      ::mmap(0, region_size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (posix_shm_addr_ == MAP_FAILED) {
    return false;
  }
  ::close(fd);
#endif
  return true;
}

bool ShmManager::Allocate() {
  switch (alloc_method_) {
    case kAllocateMappingFile: {
      return AllocateMappingFile();
    }
    case kAllocateMappingMem: {
      return AllocateMappingMem();
    }
    case kAllocateMappingDevZero: {
      return AllocateMappingDevZero();
    }
    case kAllocateSysvShmget: {
      return AllocateShmget();
    }
    case kAllocatePosixShmopen: {
      return AllocateShmopen();
    }
    default: {
      LOG(ERROR) << "not support this way.";
      break;
    }
  };
  return false;
}

bool ShmManager::Free() {
  if (alloc_method_ == kAllocateMappingFile) {
  } else if (alloc_method_ == kAllocateMappingMem ||
             alloc_method_ == kAllocateMappingDevZero) {
    if (::munmap((void *)map_addr_, size) == -1) {
      LOG(ERROR) << "munmap mem failed: " << strerror(errno);
    }
  } else if (alloc_method_ == kAllocateSysvShmget) {
    if (::shmdt(map_addr_) == -1) {
      LOG(ERROR) << "shmdt mem failed: " << strerror(errno);
    }
    // ret = shmctl(smid, IPC_RMID, NULL);
  } else if (alloc_method_ == kAllocatePosixShmopen) {
    int ret = ::munmap(posix_shm_addr_, region_size_);
    if (ret != 0) {
      return false;
    }
    ret = ::shm_unlink(shm_name_.c_str());
    if (ret != 0) {
      LOG(ERROR) << "posix shm unlink failed, errno: " << errno;
      return false;
    }
  }
  return true;
}

/*
struct sembuf {
  short semnum; ----- 信号量集合中的信号量编号，0代表第1个信号量, 1代表第二
  short val;    ----- 若val>0进行V操作信号量值加val, 表示进程释放控制的资源
                      若val<0进行P操作信号量值减val,
若(semval-val)<0(semval为该信号量值)
                        则调用进程阻塞，直到资源可用；若设置IPC_NOWAIT不会睡眠,
                        进程直接返回EAGAIN错误
                      若val==0时阻塞等待信号量为0,调用进程进入睡眠状态，直到信号值为0;
                        若设置IPC_NOWAIT, 进程不会睡眠, 直接返回EAGAIN错误
  short flag;   -----  0 设置信号量的默认操作
                        IPC_NOWAIT 设置信号量操作不等待
                        SEM_UNDO
选项会让内核记录一个与调用进程相关的UNDO记录,如果该进程崩溃
                        则根据这个进程的UNDO记录自动恢复相应信号量的计数值
};
*/
bool ShmManager::ReadLock() const {
  /* 包含两个信号量,第一个为写信号量，第二个为读信号量
   * 获取读锁
   * 等待写信号量（第一个）变为0：{0, 0, SEM_UNDO},
   * 并且把读信号量（第一个）加一：{1, 1, SEM_UNDO}
   **/
  struct sembuf sops[2] = {{0, 0, SEM_UNDO}, {1, 1, SEM_UNDO}};
  size_t nsops = 2;
  int ret = -1;
  do {
    ret = ::semop(sem_id_, &sops[0], nsops);
  } while ((ret == -1) && (errno == EINTR));
  return ret == 0;
}
bool ShmManager::WriteLock() const {
  /*  包含两个信号量,第一个为写信号量，第二个为读信号量
   * 获取写锁
   * 尝试等待写信号量（第一个）变为0：{0, 0, SEM_UNDO},
   * 并且等待读信号量（第二个）变为0：{0, 0, SEM_UNDO}
   * 把写信号量（第一个）加一：{0, 1, SEM_UNDO}
   **/
  struct sembuf sops[3] = {
      {0, 0, SEM_UNDO}, {1, 0, SEM_UNDO}, {0, 1, SEM_UNDO}};
  size_t nsops = 3;
  int ret = -1;
  do {
    ret = ::semop(sem_id_, &sops[0], nsops);
  } while ((ret == -1) && (errno == EINTR));
  return ret == 0;
}
bool ShmManager::TryReadLock() const {
  /* 包含两个信号量,第一个为写信号量，第二个为读信号量
   * 获取读锁
   * 尝试等待写信号量（第一个）变为0：{0, 0,SEM_UNDO | IPC_NOWAIT},
   * 把读信号量（第一个）加一：{1, 1,SEM_UNDO | IPC_NOWAIT}
   **/
  struct sembuf sops[2] = {{0, 0, SEM_UNDO | IPC_NOWAIT},
                           {1, 1, SEM_UNDO | IPC_NOWAIT}};
  size_t nsops = 2;

  int ret = ::semop(sem_id_, &sops[0], nsops);
  if (ret == -1) {
    if (errno == EAGAIN) {
      return false;
    } else {
      throw std::runtime_error("semop error : " + std::string(strerror(errno)));
    }
  }
  return true;
}
bool ShmManager::TryWriteLock() const {
  /*包含两个信号量,第一个为写信号量，第二个为读信号量
   *尝试获取写锁
   *尝试等待写信号量（第一个）变为0：{0, 0, SEM_UNDO |
   *IPC_NOWAIT},并且尝试等待读信号量（第二个）变为0：{0, 0, SEM_UNDO |
   *IPC_NOWAIT}
   *把写信号量（第一个）加一：{0, 1, SEM_UNDO | IPC_NOWAIT}
   **/
  struct sembuf sops[3] = {{0, 0, SEM_UNDO | IPC_NOWAIT},
                           {1, 0, SEM_UNDO | IPC_NOWAIT},
                           {0, 1, SEM_UNDO | IPC_NOWAIT}};
  size_t nsops = 3;
  int ret = ::semop(sem_id_, &sops[0], nsops);
  if (ret == -1) {
    if (errno == EAGAIN) {
      return false;
    } else {
      throw std::runtime_error("semop error : " + std::string(strerror(errno)));
    }
  }
  return true;
}
bool ShmManager::UnReadLock() const {
  /*  包含两个信号量,第一个为写信号量，第二个为读信号量
   * 解除读锁
   * 把读信号量（第二个）减一：{1, -1, SEM_UNDO}
   **/
  struct sembuf sops[1] = {{1, -1, SEM_UNDO}};
  size_t nsops = 1;
  int ret = -1;
  do {
    ret = ::semop(sem_id_, &sops[0], nsops);
  } while ((ret == -1) && (errno == EINTR));
  return ret == 0;
}
bool ShmManager::UnWriteLock() const {
  /* 包含两个信号量,第一个为写信号量，第二个为读信号量
   * 解除写锁
   *  把写信号量（第一个）减一：{0, -1, SEM_UNDO}
   **/
  struct sembuf sops[1] = {{0, -1, SEM_UNDO}};
  size_t nsops = 1;
  int ret = -1;
  do {
    ret = ::semop(sem_id_, &sops[0], nsops);
  } while ((ret == -1) && (errno == EINTR));
  return ret == 0;
}

bool ShmManager::PosixShmLock(const void *ptr, size_t size) {
  return ShmManager::MemLock(ptr, size);
}

bool ShmManager::PosixShmUnLock(const void *ptr, size_t size) {
  return ShmManager::MemUnLock(ptr, size);
}

bool ShmManager::LockReadWrite() const { return true; }
bool ShmManager::UnLockReadWrite() const { return true; }
bool ShmManager::TryLockReadWrite() const { return true; }

// https://blog.csdn.net/wangpengqi/article/details/16341935?utm_medium=distribute.pc_relevant.none-task-blog-baidulandingword-7&spm=1001.2101.3001.4242
// https://linux.die.net/man/2/mlockall
// https://blog.csdn.net/zhjutao/article/details/8652252
bool ShmManager::MemLock(const void *ptr, size_t size) {
  return ::mlock(ptr, size) == 0;
}

// madvise
// posix_madvise

// mincore
// mremap
// remap_file_pages

bool ShmManager::MemUnLock(const void *ptr, size_t size) {
  return ::munlock(ptr, size) == 0;
}

// mprotect()函数可以用来修改一段指定内存区域的保护属性
// https://www.cnblogs.com/ims-/p/13222243.html
// Linux C/C++内存越界定位: 利用mprotect使程序在crash在第一现场
// https://blog.csdn.net/thisinnocence/article/details/80025064
// 多线程内存问题分析之mprotect方法
// https://blog.csdn.net/agwtpcbox/article/details/53230664
// 文件映射IO：mmap-mprotect-msync-munmap函数族
// https://blog.csdn.net/xiaofei0859/article/details/5871148?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromBaidu-4.control&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromBaidu-4.control
bool ShmManager::MemProtect(void *ptr, size_t size, int flag) {
  return ::mprotect(ptr, size, flag) == 0;
}

// https://www.man7.org/linux/man-pages/man2/msync.2.html
// https://blog.csdn.net/todd911/article/details/6572341
// https://blog.csdn.net/xiaocaichonga/article/details/7759182
bool ShmManager::MemSync(void *ptr, size_t size, int flag) {
  return ::msync(ptr, size, flag) == 0;
}

bool ShmManager::MemLockAll(int flags) { return ::mlockall(flags) == 0; }
bool ShmManager::MemUnLockAll(int flags) { return ::munlockall() == 0; }

// brk(), sbrk() 用法详解
// https://www.cnblogs.com/zhangxian/articles/5062503.html

}  // namespace MSF