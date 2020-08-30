
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
#include "Shm.h"

#include <butil/logging.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>

#include "File.h"

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
    mmap可以将磁盘文件映射到内存中，直接操作内存时Linux内核将负责同步内存和磁盘文件中的数据，
    fd参数就指向需要同步的磁盘文件，而offset则代表从文件的这个偏移量处开始共享，当然Nginx没有使用这一特性。
    当flags参数中加入MAP
   ANON或者MAP—ANONYMOUS参数时表示不使用文件映射方式，这时fd和offset参数就没有意义，
    也不需要传递了，此时的mmap方法和ngx_shm_alloc的功能几乎完全相同。length参数就是将要在内存中开辟的线性地址空间大小，而prot参数则是操作这段共享内存的方式（如只读或者可读可写），start参数说明希望的共享内存起始映射地址，当然，通常都会把start设为NULL空指针

    先来看看如何使用mmap实现ngx_shm_alloc方法，代码如下。
    ngx_int_t ngx_shm_ alloc (ngx_shm_t *shm)
    {
        ／／开辟一块shm- >size大小且可以读／写的共享内存，内存首地址存放在addr中
        shm->addr=  (uchar *)mmap (NULL,  shm->size,
        PROT_READ l PROT_WRITE,
        MAP_ANONIMAP_SHARED,  -1,o);
    if (shm->addr == MAP_FAILED)
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
        ／／使用ngx_shm_t中的addr和size参数调用munmap释放共享内存即可
        if  (munmap( (void★)  shm->addr,  shm- >size)  ==~1)  (
        ngx_LOG(ERROR) (NGX—LOG__  ALERT,  shm- >log,
            ngx_errno,  ”munmap(%p,  %uz)failed",  shm- >addr,   shm- >size)j
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

MSF_SHM::MSF_SHM(const std::string &name, uint64_t size) {}

int MSF_SHM::ShmAllocByMapFile(const std::string &filename) {
  addr = (uint8_t *)mmap(NULL, size, PROT_READ | PROT_WRITE,
                         MAP_ANON | MAP_SHARED, -1, 0);

  if (addr == MAP_FAILED) {
    return -1;
  }
  return 0;
}

int MSF_SHM::ShmAlloc() {
  if (strategy == ALLOC_BY_MAPPING_FILE) {
  } else if (strategy == ALLOC_BY_UNMAPPING_FILE) {
    // https://blog.csdn.net/qq_33611327/article/details/81738195
    addr = (uint8_t *)mmap(NULL, size, PROT_READ | PROT_WRITE,
                           MAP_ANON | MAP_SHARED, -1, 0);

    // addr = (uint8_t *) mmap(NULL, size, PROT_READ | PROT_WRITE,
    //     MAP_PRIVATE | MAP_ANONYMOUS |
    //     MAP_POPULATE | MAP_HUGETLB, -1, 0);

    if (addr == MAP_FAILED) {
      return -1;
    }
  } else if (strategy == ALLOC_BY_MAPPING_DEVZERO) {
    int fd = open("/dev/zero", O_RDWR);
    if (fd == -1) {
      return -1;
    }
    addr =
        (uint8_t *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
      LOG(ERROR) << "mmap(/dev/zero, MAP_SHARED, " << size << "failed";
    }
    if (close(fd) == -1) {
    }
    return (addr == MAP_FAILED) ? -1 : 0;

  } else if (strategy == ALLOC_BY_SHMGET) {
    int id = shmget(IPC_PRIVATE, size, (SHM_R | SHM_W | IPC_CREAT));
    if (id == -1) {
      return -1;
    }

    LOG(INFO) << "shmget id: " << id;

    addr = (uint8_t *)shmat(id, NULL, 0);
    if (addr == (void *)-1) {
      LOG(ERROR) << "shmat mem failed: " << strerror(errno);
    }
    if (shmctl(id, IPC_RMID, NULL) == -1) {
      LOG(ERROR) << "shmctl mem failed: " << strerror(errno);
    }
    return (addr == (void *)-1) ? -1 : 0;
  } else {
    LOG(ERROR) << "Not support this way.";
    return -1;
  }
  return 0;
}

void MSF_SHM::ShmFree() {
  if (strategy == ALLOC_BY_MAPPING_FILE) {
  } else if (strategy == ALLOC_BY_UNMAPPING_FILE ||
             strategy == ALLOC_BY_MAPPING_DEVZERO) {
    if (munmap((void *)addr, size) == -1) {
      LOG(ERROR) << "munmap mem failed: " << strerror(errno);
    }
  } else if (strategy == ALLOC_BY_SHMGET) {
    if (shmdt(addr) == -1) {
      LOG(ERROR) << "shmdt mem failed: " << strerror(errno);
    }
  }
}

}  // namespace MSF