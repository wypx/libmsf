/*
 * FreeBSD file AIO features and quirks:
 *
 * if an asked data are already in VM cache, then aio_error() returns 0,
 * and the data are already copied in buffer;
 *
 * aio_read() preread in VM cache as minimum 16K (probably BKVASIZE);
 * the first AIO preload may be up to 128K;
 *
 * aio_read/aio_error() may return EINPROGRESS for just written data;
 *
 * kqueue EVFILT_AIO filter is level triggered only: an event repeats
 * until aio_return() will be called;
 *
 * aio_cancel() cannot cancel file AIO: it returns AIO_NOTCANCELED always.
 */

class MSF_CACHE {
 public:
#if (HAVE_F_READAHEAD)
#define ReadAHead(fd, n) ::fcntl(fd, F_READAHEAD, (int)n)
#elif(HAVE_POSIX_FADVISE)
  int ReadAHead(int fd, size_t n) {
    int err;
    /*
      * Thishttps://blog.csdn.net/skdkjzz/article/details/46473207
      * https://www.cnblogs.com/aquester/p/9891632.html
      * https://www.yiibai.com/unix_system_calls/posix_fadvise.html
      * */
    err = ::posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
    if (err == 0) {
      return 0;
    }

    errno = err;
    return -1;
  }
#else
  // https://linux.die.net/man/2/readahead
  int ReadAHead(int fd, size_t n) { return ::readahead(fd, 0, n); }
#endif
};

class MSF_AIO {
 public:
/* direct AIO可以参考http://blog.csdn.net/bengda/article/details/21871413
 * 普通缓存I/O优点:
 * 缓存 I/O
 * 使用了操作系统内核缓冲区,在一定程度上分离了应用程序空间和实际的物理设备. 缓存
 * I/O 可以减少读盘的次数, 从而提高性能. 普通缓存I/O优点缺点: 在缓存 I/O 机制中,
 * DMA 方式可以将数据直接从磁盘读到页缓存中,
 * 或者将数据从页缓存直接写回到磁盘上,而不能直接在应用程序地址空间和磁盘之间进行数据传输,
 * 这样的话,数据在传输过程中需要在应用程序地址空间和页缓存之间进行多次数据拷贝操作,
 * 这些数据拷贝操作所带来的 CPU 以及内存开销是非常大的.
 *
 * direct I/O优点:
 * 直接 I/O
 * 最主要的优点就是通过减少操作系统内核缓冲区和应用程序地址空间的数据拷贝次数,
 * 降低了对文件读取和写入时所带来的 CPU 的使用以及内存带宽的占用.
 * 这对于某些特殊的应用程序，比如自缓存应用程序来说，不失为一种好的选择.
 * 如果要传输的数据量很大,使用直接 I/O 的方式进行数据传输,
 * 而不需要操作系统内核地址空间拷贝数据操作的参与,这将会大大提高性能.
 * direct I/O缺点:
 * 设置直接 I/O 的开销非常大,而直接 I/O 又不能提供缓存 I/O 的优势.
 * 缓存 I/O 的读操作可以从高速缓冲存储器中获取数据,而直接I/O
 * 的读数据操作会造成磁盘的同步读, 这会带来性能上的差异,
 * 并且导致进程需要较长的时间才能执行完;
 */
#if (MSF_HAVE_O_DIRECT)
  int EnableDirectIO(int fd) {
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1) {
      return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_DIRECT);
  }
  int DisbleDirectIO(int fd) {
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1) {
      return -1;
    }
    return fcntl(fd, F_SETFL, flags & ~O_DIRECT);
  }
#elif(MSF_HAVE_F_NOCACHE)
  int EnableDirectIO(int fd) { return fcntl(fd, F_NOCACHE, 1); }
#elif(MSF_HAVE_DIRECTIO)
  int EnableDirectIO(int fd) { return directio(fd, DIRECTIO_ON); }
#else
#define EnableDirectIO(fd) ()
#endif

 private:
};

size_t msf_fs_bsize(u8 *name);

// tee
// splice
// vmsplice
// sync_file_range
// readahead

// https://blog.csdn.net/ysh1042436059/article/details/84955188
// open_by_handle_at
// name_to_handle_at