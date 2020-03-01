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
#include <base/File.h>
#include <base/GccAttr.h>
#include <base/Logger.h>

#include <cstdio>
#include <queue>

using namespace MSF::BASE;
using namespace MSF::IO;

namespace MSF {
namespace IO {

/* realpath是用来将参数path所指的相对路径转换成绝对路径
 * 如果resolved_path为NULL，则该函数调用malloc分配一块大小为PATH_MAX的内存
 * 来存放解析出来的绝对路径，并返回指向这块区域的指针, 主动free这块内存 */
std::string GetRealPath(const std::string& path) {
  char realPath[256] = {0};
  if (!realpath(path.c_str(), realPath)) {
    // MSF_ERROR << "Get real path failed";
  }
  return std::string(realPath);
}

/* 如果 size太小无法保存该地址，返回 NULL 并设置 errno 为 ERANGE */
std::string GetCurrentWorkDir() {
  char workPath[256] = {0};
  if (!getcwd(workPath, 0) && errno == ERANGE) {
    MSF_ERROR << "Get current working directory failed";
  }
  return std::string(workPath);
}

bool CreateDir(const std::string& path, const mode_t mode) {
#ifdef WIN32
  return _mkdir(path.c_str(), mode) == 0;
#else
  return mkdir(path.c_str(), mode) == 0;
#endif
}

bool DeleteDir(const std::string& path) { return rmdir(path.c_str()) == 0; }

bool CreateDirWithUid(const std::string& path, const mode_t mode, uid_t user) {
  if (mkdir(path.c_str(), mode) < 0) {
    MSF_ERROR << "Fail to create dir: " << path;
    return false;
  }
  struct stat fi;
  if (stat(path.c_str(), &fi) == -1) {
    return false;
  }

  if (fi.st_uid != user) {
    if (chown(path.c_str(), user, -1) == -1) {
      return false;
    }
  }

  if ((fi.st_mode & (S_IRUSR | S_IWUSR | S_IXUSR)) !=
      (S_IRUSR | S_IWUSR | S_IXUSR)) {
    fi.st_mode |= (S_IRUSR | S_IWUSR | S_IXUSR);
    if (chmod(path.c_str(), fi.st_mode) == -1) {
      return false;
    }
  }
  return true;
}
// void CreateFileDir(const std::string & path)
// {
//     path.replace('/', '\\');
//     int nPos = path.find('\\');
//     while (nPos != -1) {
//         CreateDirectory(path.at(nPos), nullptr);
//         nPos = path.find('\\', nPos + 1);
//     }
// }

// https://blog.csdn.net/rathome/article/details/78870694
bool CreateFullDir(const std::string& path, const mode_t mode) {
  std::string s = path;
  size_t pre = 0, pos;
  std::string dir;

  if (s[s.size() - 1] != '/') {
    /* force trailing / so we can handle everything in loop */
    s += '/';
  }
  while ((pos = s.find_first_of('/', pre)) != std::string::npos) {
    dir = s.substr(0, pos++);
    pre = pos;
    if (dir.size() == 0) {
      continue;  // if leading / first time is 0 length
    }
    if (mkdir(dir.c_str(), mode) < 0 && errno != EEXIST) {
      return false;
    }
  }
  if (IsDirsExist(path)) {
    return true;
  } else {
    return false;
  }
}

bool IsFileExist(const std::string& file) {
#ifdef WIN32
  return (_access(file.c_str(), R_OK | W_OK) >= 0);
#else
  return (access(file.c_str(), R_OK | W_OK) >= 0);
#endif
}

bool IsDir(const struct stat st) { return S_ISDIR(st.st_mode); }

bool IsFile(const struct stat st) { return S_ISREG(st.st_mode); }
bool IsLink(const struct stat st) { return S_ISLNK(st.st_mode); }

int DirAccess(const struct stat st) { return st.st_mode & 0777; }

int DirSize(const struct stat st) { return st.st_size; }
int DirFsSize(const struct stat st) {
  return std::max(st.st_size, st.st_blocks * 512);
}
/* time of last modification */
time_t DirMtime(const struct stat st) { return st.st_mtime; }

bool IsDirsExist(const std::string& dir) {
  // https://www.cnblogs.com/Anker/p/3349672.html
  struct stat fi;

  // lstat
  if (stat(dir.c_str(), &fi) == -1) {
    if (errno == ENOENT) {
      // MSF_ERROR << "Dir not exist:" << dir;
    }
    return false;
  }
  return true;
}

void ListFiles(std::list<std::string>& list, const std::string& folder,
               const std::string& extension, bool recursive) {
  DIR* dir;
  DIR* subDir;
  struct dirent* ent;
  // try to open top folder
  dir = opendir(folder.c_str());
  if (dir == NULL) {
    // could not open directory
    MSF_ERROR << "Could not open \"" << folder << "\" directory.";
    return;
  } else {
    // close, we'll process it next
    closedir(dir);
  }
  // enqueue top folder
  std::queue<std::string> folders;
  folders.push(folder);

  // run while has queued folders
  while (!folders.empty()) {
    std::string currFolder = folders.front();
    folders.pop();
    dir = opendir(currFolder.c_str());
    if (dir == NULL) continue;
    // iterate through all the files and directories
    while ((ent = readdir(dir)) != NULL) {
      std::string name(ent->d_name);

      // int dirFd = dirfd(dir);
      // int type = de->d_type;
      // if (strstr(de->d_name, "xxxx") != nullptr) {
      //     MSF_INFO << "delete /dev/shm/" << de->d_name;
      //     unlinkat(dirFd, de->d_name, 0);
      // }
      // ignore "." and ".." directories
      if (name.compare(".") == 0 || name.compare("..") == 0) continue;
      // add path to the file name
      std::string path = currFolder;
      path.append("/");
      path.append(name);
      // check if it's a folder by trying to open it
      subDir = opendir(path.c_str());
      if (subDir != NULL) {
        // it's a folder: close, we can process it later
        closedir(subDir);
        if (recursive) folders.push(path);
      } else {
        // it's a file
        if (extension.empty()) {
          list.push_back(path);
        } else {
          // check file extension
          size_t lastDot = name.find_last_of('.');
          std::string ext = name.substr(lastDot + 1);
          if (ext.compare(extension) == 0) {
            // match
            list.push_back(path);
          }
        }  // endif (extension test)
      }    // endif (folder test)
    }      // endwhile (nextFile)
    closedir(dir);
  }  // endwhile (queued folders)

}  // end listFiles

#ifndef HAVE_LINK
#ifdef WIN32
int link(const char* path1, const char* path2) {
  if (CreateHardLink(path2, path1, NULL) == 0) {
    /* It is not documented which errors CreateHardLink() can produce.
     * The following conversions are based on tests on a Windows XP SP2
     * system. */
    DWORD err = GetLastError();
    switch (err) {
      case ERROR_ACCESS_DENIED:
        errno = EACCES;
        break;

      case ERROR_INVALID_FUNCTION: /* fs does not support hard links */
        errno = EPERM;
        break;

      case ERROR_NOT_SAME_DEVICE:
        errno = EXDEV;
        break;

      case ERROR_PATH_NOT_FOUND:
      case ERROR_FILE_NOT_FOUND:
        errno = ENOENT;
        break;

      case ERROR_INVALID_PARAMETER:
        errno = ENAMETOOLONG;
        break;

      case ERROR_TOO_MANY_LINKS:
        errno = EMLINK;
        break;

      case ERROR_ALREADY_EXISTS:
        errno = EEXIST;
        break;

      default:
        errno = EIO;
    }
    return -1;
  }

  return 0;
}
#endif
#endif

std::string GetFileName(const std::string& path) {
  char ch = '/';

#ifdef _WIN32
  ch = '\\';
#endif

  size_t pos = path.rfind(ch);
  if (pos == std::string::npos)
    return path;
  else
    return path.substr(pos + 1);
}

File::File() noexcept : fd_(-1), ownsFd_(false) {}

File::File(int fd, bool ownsFd) noexcept : fd_(fd), ownsFd_(ownsFd) {
  if (fd != -1 || !ownsFd) {
    std::cout << "cannot own -1";
  }
}

File::File(const char* name, int flags, mode_t mode)
    : fd_(::open(name, flags, mode)), ownsFd_(false) {
  if (fd_ == -1) {
    std::cout << "Open file failed" << std::endl;
  }
  ownsFd_ = true;
}

File::File(const std::string& name, int flags, mode_t mode)
    : File(name.c_str(), flags, mode) {}

File::File(File&& other) noexcept : fd_(other.fd_), ownsFd_(other.ownsFd_) {
  other.release();
}

File& File::operator=(File&& other) {
  closeNoThrow();
  swap(other);
  return *this;
}

File::~File() {
  auto fd = fd_;
  if (!closeNoThrow()) {  // ignore most errors
    MSF_ERROR
        << "closing fd " << fd << ", it may already "
        << "have been closed. Another time, this might close the wrong FD.";
  }
}

File File::temporary() {
  // make a temp file with tmpfile(), dup the fd, then return it in a File.
  FILE* tmpFile = tmpfile();
  if (!tmpFile) {
    MSF_ERROR << "tmpfile() failed";
  }

  int fd = ::dup(fileno(tmpFile));
  if (fd < 0) {
    MSF_ERROR << "dup() failed";
  }

  return File(fd, true);
}

int File::release() noexcept {
  int released = fd_;
  fd_ = -1;
  ownsFd_ = false;
  return released;
}

void File::swap(File& other) noexcept {
  std::swap(fd_, other.fd_);
  std::swap(ownsFd_, other.ownsFd_);
}

void swap(File& a, File& b) noexcept { a.swap(b); }

File File::dup() const {
  if (fd_ != -1) {
    int fd = ::dup(fd_);
    if (fd < 0) {
      std::cout << "dup() failed" << std::endl;
    }

    return File(fd, true);
  }

  return File();
}

void File::close() {
  if (!closeNoThrow()) {
    std::cout << "close() failed" << std::endl;
  }
}

bool File::closeNoThrow() {
  int r = ownsFd_ ? ::close(fd_) : 0;
  release();
  return r == 0;
}

void File::lock() { doLock(LOCK_EX); }
bool File::try_lock() { return doTryLock(LOCK_EX); }
void File::lock_shared() { doLock(LOCK_SH); }
bool File::try_lock_shared() { return doTryLock(LOCK_SH); }

void File::doLock(int op) {
  if (flockNoInt(fd_, op) < 0) {
    std::cout << "flock() failed (lock)" << std::endl;
  }
}

bool File::doTryLock(int op) {
  int r = flockNoInt(fd_, op | LOCK_NB);
  // flock returns EWOULDBLOCK if already locked
  if (r == -1 && errno == EWOULDBLOCK) {
    return false;
  }

  if (unlikely(r == -1)) {
    std::cout << "flock() failed (try_lock)" << std::endl;
  }
  return true;
}

void File::unlock() {
  if (flockNoInt(fd_, LOCK_UN) < 0) {
    std::cout << "flock() failed (unlock)" << std::endl;
  }
}
void File::unlock_shared() { unlock(); }

pid_t ReadPidFile(const std::string& pid_file) {
  FILE* pid_fp = NULL;
  pid_t pid = -1;
  int i;

  pid_fp = fopen(pid_file.c_str(), "r");
  if (pid_fp != NULL) {
    pid = 0;
    if (fscanf(pid_fp, "%d", &i) == 1) pid = (pid_t)i;
    sfclose(pid_fp);
  } else {
    if (errno != ENOENT) {
      MSF_ERROR << "error: could not read pid file, " << pid_file << ":"
                << strerror(errno);
      exit(1);
    }
  }
  return pid;
}

int CheckRunningPid(const std::string& pid_file) {
  pid_t pid;
  pid = ReadPidFile(pid_file);
  if (pid < 2) return 0;
  if (kill(pid, 0) < 0) return 0;
  MSF_DEBUG << "process is already running!  process id: " << (long int)pid;
  return 1;
}

void WritePidFile(const std::string& pid_file) {
  FILE* fp = NULL;
  fp = fopen(pid_file.c_str(), "w+");
  if (!fp) {
    MSF_ERROR << "could not write pid file =>" << pid_file << strerror(errno);
    return;
  }
  fprintf(fp, "%d\n", getpid());
  fclose(fp);
}

char* config_read_file(const char* filename) {
  FILE* file = NULL;
  int64_t length = 0;
  char* content = NULL;
  size_t read_chars = 0;

  /* open in read binary mode */
  file = fopen(filename, "rb");
  if (!file) {
    goto cleanup;
  }

  /* get the length */
  if (fseek(file, 0, SEEK_END) != 0) {
    goto cleanup;
  }

  length = ftell(file);
  if (length < 0) {
    goto cleanup;
  }

  if (fseek(file, 0, SEEK_SET) != 0) {
    goto cleanup;
  }

  /* allocate content buffer */
  content = (char*)malloc((size_t)length + 1);
  if (!content) {
    goto cleanup;
  }

  memset(content, 0, (size_t)length + 1);

  /* read the file into memory */
  read_chars = fread(content, sizeof(char), (size_t)length, file);
  if ((long)read_chars != length) {
    free(content);
    goto cleanup;
  }

  content[read_chars] = '\0';

cleanup:
  fclose(file);
  return content;
}

}  // namespace IO
}  // namespace MSF