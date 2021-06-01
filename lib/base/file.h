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
#ifndef BASE_FILE_H_
#define BASE_FILE_H_

#include <dirent.h>
#include <fcntl.h>
#include <glob.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/time.h> /* for utimes */
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <utime.h> /* for utime */

#include <algorithm>
#include <iostream>
#include <list>
#include <string>

#ifdef WIN32
#include <direct.h>
#include <io.h>
#endif

#include <base/logging.h>

#include "file_utils.h"

// https://github.com/philipl/pifs/tree/master/src

using namespace MSF;

namespace MSF {

std::string GetRealPath(const std::string& path);

std::string GetCurrentWorkDir();

bool IsFileExist(const std::string& file);
bool IsDirsExist(const std::string& dir);

bool CreateDir(const std::string& path, const mode_t mode = 0755);
bool DeleteDir(const std::string& path);
bool CreateDirWithUid(const std::string& path, const mode_t mode, uid_t user);
bool CreateFullDir(const std::string& path, const mode_t mode = 0755);

void ListFiles(std::list<std::string>& list, const std::string& folder,
               const std::string& extension, bool recursive);

/*
 * link() - only for systems that lack it
 */
#ifndef HAVE_LINK
int link(const char* path1, const char* path2);
#endif

class File {
 public:
  /* 关键字告诉编译器, 函数中不会发生异常,这有利于编译器对程序做更多的优化 */
  File() noexcept;
  explicit File(int fd, bool ownsFd = false) noexcept;
  explicit File(const char* name, int flags = O_RDONLY, mode_t mode = 0666);
  explicit File(const std::string& name, int flags = O_RDONLY,
                mode_t mode = 0666);
  // explicit File(StringPiece name, int flags = O_RDONLY, mode_t mode = 0666);

  /**
   * All the constructors that are not noexcept can throw std::system_error.
   * This is a helper method to use folly::Expected to chain a file open event
   * to something else you want to do with the open fd.
   */

  // template <typename... Args>
  // static File makeFile(Args&&... args) noexcept {
  //     try {
  //         return File(std::forward<Args>(args)...);
  //     } catch (const std::system_error& se) {
  //         std::cout << "Caught system_error with code " << se.code()
  //           << " meaning " << se.what() <<" current se:"
  //           << std::current_exception() << std::endl;
  //     }
  // }

  ~File();

  /** Create and return a temporary, owned file (uses tmpfile()). */
  static File temporary();

  /** Return the file descriptor, or -1 if the file was closed. */
  int fd() const { return fd_; }

  /** Returns 'true' iff the file was successfully opened. */
  explicit operator bool() const { return fd_ != -1; }

  /** Duplicate file descriptor and return File that owns it. */
  File dup() const;

  /**
   * If we own the file descriptor, close the file and throw on error.
   * Otherwise, do nothing.
   */
  void close();

  /**
   * Closes the file (if owned).  Returns true on success, false (and sets
   * errno) on error.
   */
  bool closeNoThrow();

  /**
   * Returns and releases the file descriptor; no longer owned by this File.
   * Returns -1 if the File object didn't wrap a file.
   */
  int release() noexcept;

  /** Swap this File with another. */
  void swap(File& other) noexcept;

  // movable
  File(File&&) noexcept;
  File& operator=(File&&);

  void lock();
  bool try_lock();
  void unlock();

  void lock_shared();
  bool try_lock_shared();
  void unlock_shared();

  int OpenFile(const std::string& filename, const int mode, const int create,
               const int access) {
    return open(filename.c_str(), mode, create, access);
  }

  struct MSF_FILEMAP {
    char* name;
    size_t size;
    void* addr;
    int fd;
  };

  bool CreateFileMap(struct MSF_FILEMAP* fm) {
    fm->fd = OpenFile(fm->name, O_RDWR, O_CREAT | O_TRUNC, 0644);
    if (ftruncate(fm->fd, fm->size) == -1) {
      LOG(ERROR) << "ftruncate file failed:" << fm->name;
    }
    fm->addr =
        mmap(NULL, fm->size, PROT_READ | PROT_WRITE, MAP_SHARED, fm->fd, 0);
    if (fm->addr != MAP_FAILED) {
      ::close(fm->fd);
      return false;
    }
    return true;
  }

  bool CloseFileMap(struct MSF_FILEMAP* fm) {
    if (munmap(fm->addr, fm->size) == -1) {
      LOG(ERROR) << "munmap failed";
      return false;
    }
    ::close(fm->fd);
    return true;
  }

  void CloseFile() {
    if (fd_ > 0) {
      ::close(fd_);
      fd_ = -1;
    }
  }
  /* unlink 只能删除文件,不能删除目录
   * unlink函数使文件引用数减一,当引用数为零时,操作系统就删除文件.
   * 但若有进程已经打开文件,则只有最后一个引用该文件的文件描述符关闭,
   * 该文件才会被删除 */

  int DeleteFile(const std::string& filename) {
    return unlink(filename.c_str());
  }

  /* rename函数功能是给一个文件重命名,用该函数可以实现文件移动功能
   * 把一个文件的完整路径的盘符改一下就实现了这个文件的移动
   * rename和mv命令差不多,只是rename支持批量修改
   * mv也能用于改名,但不能实现批量处理(改名时,不支持*等符号的)而rename可以
   * rename后文件的fd和stat信息不变使用 RENAME,原子性地对临时文件进行改名
   * 覆盖原来的 RDB 文件*/
  int RenameFile(const std::string& dstFile, const std::string& srcFile) {
    return rename(dstFile.c_str(), srcFile.c_str());
  }

  int ChangeFileAccess(const std::string& filename, int access) {
    return chmod(filename.c_str(), access);
  }

  int getFileInfo(const std::string& path) {
    //  struct stat f, target;
    /* Stat right resource */
    if (LStatFile(path, &st) == -1) {
      if (errno == EACCES) {
        // int is_exist = true;
      }
      // MSF_FILE_LOG(DBG_ERROR, "Lstat path(%s) failed.", path);
      return -1;
    }

    // bool exist = true;
    // bool file = true;
    // bool link = false;
    // bool dir = false;
    exec_access = false;
    read_access = false;

    if (is_link()) {
      // link = true;
      // file = false;
      if (StatFile(path, &st) == -1) {
        // MSF_FILE_LOG(DBG_ERROR, "Stat path(%s) failed.", path);
        return -1;
      }
    }
    file_size();
    file_mtime();

    if (is_dir()) {
      // dir  = true;
      // file = false;
    }

    is_readable();
    is_writable();
    /* Suggest open(2) flags */
    flags_read_only = O_RDONLY | O_NONBLOCK;

#if defined(__linux__)
    // gid_t EGID = getegid();
    gid_t EUID = geteuid();
    /*
     * If the user is the owner of the file or the user is root, it
     * can set the O_NOATIME flag for open(2) operations to avoid
     * inode updates about last accessed time
     */
    if (st.st_uid == EUID || EUID == 0) {
      flags_read_only |= O_NOATIME;
    }
#endif
  }

  /* Stat right resource */
  int LStatFile(const std::string& filename) {
    return lstat(filename.c_str(), &st);
  }
  /*  stat系统调用系列包括了fstat,stat和lstat x*/
  int StatFile(const std::string& filename, struct stat* st) {
    return stat(filename.c_str(), st);
  }

  int FStatFile(int fd, struct stat* st) { return fstat(fd, st); }

  struct stat st;

  inline bool is_dir() const { return S_ISDIR(st.st_mode); }
  inline bool is_file() const { return S_ISREG(st.st_mode); }
  inline bool is_link() const { return S_ISLNK(st.st_mode); }
  inline bool is_executable() const {
    return ((st.st_mode & S_IXUSR) == S_IXUSR);
  }
  inline int file_access() const { return st.st_mode & 0777; }

  bool is_readable() const {
    gid_t EGID = getegid();
    gid_t EUID = geteuid();

    /* Check read access */
    if (((st.st_mode & S_IRUSR) && st.st_uid == EUID) ||
        ((st.st_mode & S_IRGRP) && st.st_gid == EGID) ||
        (st.st_mode & S_IROTH)) {
      return true;
    }
    return false;
  }

  bool is_writable() const {
    gid_t EGID = getegid();
    gid_t EUID = geteuid();

    if ((st.st_mode & S_IXUSR && st.st_uid == EUID) ||
        (st.st_mode & S_IXGRP && st.st_gid == EGID) || (st.st_mode & S_IXOTH)) {
      return true;
    }
    return false;
  }
  /* total size, in bytes */
  inline int file_size() const { return st.st_size; }
  inline int file_fs_size() const {
    return std::max(st.st_size, st.st_blocks * 512);
  }
  /* time of last modification */
  inline time_t file_mtime() const { return st.st_mtime; }
  /* inode number -inode节点号, 同一个设备中的每个文件, 这个值都是不同的*/
  inline ino_t file_uniq() const { return st.st_ino; }

  int setFileTime(const char* name, int fd, time_t s) {
    struct timeval tv[2];
    tv[0].tv_usec = 0;
    tv[1].tv_sec = s;
    tv[1].tv_usec = 0;

    if (utimes(name, tv) != -1) {
      return -1;
    }
    return 0;
  }

  int LStatFile(const std::string& filename, struct stat* st) {
    return lstat(filename.c_str(), st);
  }

  ssize_t ReadFile(int fd, char* buf, size_t size, off_t offset) {
    ssize_t n;

#if (MSF_HAVE_PREAD)
    n = pread(fd, buf, size, offset);
    if (n == -1) {
      return -1;
    }
#else
    n = read(fd, buf, size);
    if (n == -1) {
      return -1;
    }
#endif
    return n;
  }

  ssize_t WriteFile(int fd, char* buf, size_t size, off_t offset) {
    ssize_t n, written = 0;

#if (MSF_HAVE_PWRITE)
    for (;;) {
      /* pwrite() 把缓存区 buf 开头的 count 个字节写入文件描述符
       * fd offset 偏移位置上,文件偏移没有改变*/
      n = pwrite(fd, buf + written, size, offset);
      if (n == -1) {
        return -1;
      }

      if ((size_t)n == size) {
        return written;
      }

      offset += n;
      size -= n;
    }
#else
    for (;;) {
      n = write(fd, buf + written, size);
      if (n == -1) {
        return -1;
      }
    }
#endif
  }

#define MSF_HAVE_STATFS 1
#if (MSF_HAVE_STATFS)
  size_t FileSysBlockSize(const std::string& dev) {
    struct statfs fs;
    if (statfs(dev.c_str(), &fs) == -1) {
      return 512;
    }
    if ((fs.f_bsize % 512) != 0) {
      return 512;
    }
    return (size_t)fs.f_bsize; /*每个block里面包含的字节数*/
  }
#elif(MSF_HAVE_STATVFS)
  size_t FileSysBlockSize(const std::string& dev) {
    struct statvfs fs;

    if (statvfs(dev.c_str(), &fs) == -1) {
      return 512;
    }

    if ((fs.f_frsize % 512) != 0) {
      return 512;
    }
    return (size_t)fs.f_frsize;
  }
#else
  size_t FileSysBlockSize(const std::string& dev) { return 512; }
#endif
 private:
  void doLock(int op);
  bool doTryLock(int op);

  // unique
  File(const File&) = delete;
  File& operator=(const File&) = delete;

  int fd_;
  bool ownsFd_;
  /* Suggest flags to open this file */
  bool flags_read_only;
  enum FileType {
    FTYPE_DIR,
    FTYPE_FILE,
    FTYPE_LINK
  };
  int file_type;
/*
 * -rw-rw-r--
 * 一共有10位数
 *　　其中： 最前面那个 - 代表的是类型
 *　　中间那三个 rw- 代表的是所有者（user）
 *　　然后那三个 rw- 代表的是组群（group）
 *　　最后那三个 r-- 代表的是其他人（other）*/

#define MSF_FILE_DEFAULT_ACCESS 0644
#define MSF_FILE_OWNER_ACCESS 0600

#define MSF_FILE_RDONLY O_RDONLY
#define MSF_FILE_WRONLY O_WRONLY
#define MSF_FILE_RDWR O_RDWR
#define MSF_FILE_CREATE_OR_OPEN O_CREAT
#define MSF_FILE_OPEN 0
#define MSF_FILE_TRUNCATE (O_CREAT | O_TRUNC)
#define MSF_FILE_APPEND (O_WRONLY | O_APPEND)
#define MSF_FILE_NONBLOCK O_NONBLOCK

  bool isExecutable;
  bool isDirectIO : 1;  //当文件大小大于directio xxx
  bool exec_access;
  bool read_access;
  ino_t uniq;    //文件inode节点号 同一个设备中的每个文件,这个值都是不同的
  time_t mtime;  //文件最后被修改的时间,last modify time
};

void swap(File& a, File& b) noexcept;

#define sfclose(fp)     \
  do {                  \
    if ((fp) != NULL) { \
      fclose((fp));     \
      (fp) = NULL;      \
    }                   \
  } while (0)

class MSF_GLOB {
 public:
  bool OpenGlob() {
    int n = glob(pattern, 0, NULL, &pglob);
    if (n == 0) {
      return false;
    }
#ifdef GLOB_NOMATCH
    if (n == GLOB_NOMATCH && test) {
      return true;
    }

#endif
    return true;
  }

  int ReadGlob(std::string& name) {
    size_t count;
#ifdef GLOB_NOMATCH
    count = (size_t)pglob.gl_pathc;
#else
    count = (size_t)pglob.gl_matchc;
#endif
    if (n < count) {
      // int len = (size_t) strlen(pglob.gl_pathv[n]);
      // uint8_t *data = (uint8_t *) pglob.gl_pathv[n];
      n++;
      return true;
    }
    return false;
  }

  void CloseGlob() { globfree(&pglob); }

 private:
  size_t n;
  glob_t pglob;
  char* pattern;
  bool test;
};

}  // namespace MSF
#endif
