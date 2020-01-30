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

using namespace MSF::BASE;
using namespace MSF::IO;

namespace MSF {
namespace IO {

/* realpath是用来将参数path所指的相对路径转换成绝对路径
* 如果resolved_path为NULL，则该函数调用malloc分配一块大小为PATH_MAX的内存
* 来存放解析出来的绝对路径，并返回指向这块区域的指针, 主动free这块内存 */
std::string GetRealPath(const std::string & path)
{
    char realPath[256] = { 0 };
    if (!realpath(path.c_str(), realPath)) {
        // MSF_ERROR << "Get real path failed";
    }
    return std::string(realPath);
}

/* 如果 size太小无法保存该地址，返回 NULL 并设置 errno 为 ERANGE */
std::string GetCurrentWorkDir()
{
    char workPath[256] = { 0 };
    if (!getcwd(workPath, 0) && errno == ERANGE) {
        MSF_ERROR << "Get current working directory failed";
    }
    return std::string(workPath);
}

bool CreateDir(const std::string & path, const int access)
{
    #ifdef WIN32
    return _mkdir(path.c_str(), access);
    #else
    return mkdir(path.c_str(), access);
    #endif
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

//https://blog.csdn.net/rathome/article/details/78870694
bool CreateFullDir(const std::string & path, const mode_t mode)
{
    std::string s = path;
    size_t pre = 0, pos;
    std::string dir;

    if (s[s.size()-1] != '/'){
        /* force trailing / so we can handle everything in loop */
        s += '/';
    }
    while ((pos = s.find_first_of('/', pre)) != std::string::npos) {
        dir = s.substr(0, pos++);
        pre = pos;
        if (dir.size() == 0) {
            continue; // if leading / first time is 0 length
        }
        if (mkdir(dir.c_str(), mode) < 0 && errno != EEXIST) {
            return false;
        }
    }
    if (isDirsExist(path)) {
        return true;
    } else {
        return false;
    }
}

bool isFileExist(const std::string & file)
{
    #ifdef WIN32
    return (_access(file.c_str(), R_OK | W_OK) >= 0);
    #else
    return (access(file.c_str(), R_OK | W_OK) >= 0);
    #endif
}

bool isDirsExist(const std::string & dir)
{
    //https://www.cnblogs.com/Anker/p/3349672.html
    struct stat fi;
    if (stat(dir.c_str(), &fi) == -1) {
        if (errno == ENOENT) {
            // MSF_ERROR << "Dir not exist:" << dir;
        }
        return false;
    }
    return true;
}

std::string GetFileName(const std::string & path)
{
    char ch='/';

#ifdef _WIN32
	ch='\\';
#endif

    size_t pos = path.rfind(ch);
    if (pos == std::string::npos)
        return path;
    else
        return path.substr(pos+ 1);
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
  if (!closeNoThrow()) { // ignore most errors
        MSF_ERROR << "closing fd " << fd << ", it may already "
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

void swap(File& a, File& b) noexcept {
    a.swap(b);
}

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

void File::lock() {
    doLock(LOCK_EX);
}
bool File::try_lock() {
    return doTryLock(LOCK_EX);
}
void File::lock_shared() {
  doLock(LOCK_SH);
}
bool File::try_lock_shared() {
  return doTryLock(LOCK_SH);
}

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
void File::unlock_shared() {
    unlock();
}



pid_t ReadPidFile(const std::string &pid_file) 
{
    FILE *pid_fp = NULL;
    pid_t pid = -1;
    int i;

    pid_fp = fopen(pid_file.c_str(), "r");
    if (pid_fp != NULL) {
        pid = 0;
        if (fscanf(pid_fp, "%d", &i) == 1)
            pid = (pid_t) i;
        sfclose(pid_fp);
    } else {
        if (errno != ENOENT) {
            MSF_ERROR << "error: could not read pid file, " 
                        << pid_file << ":" << strerror(errno);
            exit(1);
        }
    }
    return pid;
}


int CheckRunningPid(const std::string &pid_file)
{
    pid_t pid;
    pid = ReadPidFile(pid_file);
    if (pid < 2)
        return 0;
    if (kill(pid, 0) < 0)
        return 0;
    MSF_DEBUG << "process is already running!  process id: " << (long int) pid;
    return 1;
}

void WritePidFile(const std::string &pid_file)
{
    FILE *fp = NULL;
    fp = fopen(pid_file.c_str(), "w+");
    if (!fp) {
        MSF_ERROR << "could not write pid file =>" << pid_file << strerror(errno);
        return;
    }
    fprintf(fp, "%d\n", getpid());
    fclose(fp);
}


char *config_read_file(const char *filename) 
{
    FILE *file = NULL;
    int64_t length = 0;
    char *content = NULL;
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

} /**************************** end namespace IO   ****************************/
} /**************************** end namespace MSF  ****************************/