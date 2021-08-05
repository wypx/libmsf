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
#include "file.h"

#include <cstdio>
#include <queue>

#include "gcc_attr.h"

using namespace MSF;

namespace MSF {

#if 0
#if !defined(_WIN32)
#define _unlink unlink
#define _rmdir rmdir
#define _access access
#endif

#if defined(_WIN32)


int mkdir(const char *path, int mode) {
	return _mkdir(path);
}

DIR *opendir(const char *name) {
	char namebuf[512];
	snprintf(namebuf, sizeof(namebuf)-1, "%s\\*.*", name);

	WIN32_FIND_DATAA FindData;
    auto hFind = FindFirstFileA(namebuf, &FindData);
	if (hFind == INVALID_HANDLE_VALUE) {
        WarnL << "FindFirstFileA failed:" << get_uv_errmsg();
		return nullptr;
	}
	DIR *dir = (DIR *)malloc(sizeof(DIR));
	memset(dir, 0, sizeof(DIR));
	dir->dd_fd = 0;   // simulate return  
	dir->handle = hFind;
	return dir;
}
struct dirent *readdir(DIR *d) {
	HANDLE hFind = d->handle;
	WIN32_FIND_DATAA FileData;
	//fail or end  
	if (!FindNextFileA(hFind, &FileData)) {
		return nullptr;
	}
	struct dirent *dir = (struct dirent *)malloc(sizeof(struct dirent) + sizeof(FileData.cFileName));
    strcpy(dir->d_name, (char *)FileData.cFileName);
	dir->d_reclen = strlen(dir->d_name);
	//check there is file or directory  
	if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		dir->d_type = 2;
	}
	else {
		dir->d_type = 1;
	}
	if (d->index) {
		//覆盖前释放内存
		free(d->index);
		d->index = nullptr;
	}
	d->index = dir;
	return dir;
}
int closedir(DIR *d) {
	if (!d) {
		return -1;
	}
	//关闭句柄
	if (d->handle != INVALID_HANDLE_VALUE) {
		FindClose(d->handle);
		d->handle = INVALID_HANDLE_VALUE;
	}
	//释放内存
	if (d->index) {
		free(d->index);
		d->index = nullptr;
	}
	free(d);
	return 0;
}
#endif  // defined(_WIN32)


namespace toolkit {

// fmemopen

FILE *File::createfile_file(const char *file, const char *mode) {
	std::string path = file;
	std::string dir;
	int index = 1;
	FILE *ret = NULL;
	while (true) {
		index = path.find('/', index) + 1;
		dir = path.substr(0, index);
		if (dir.length() == 0) {
			break;
		}
		if (_access(dir.c_str(), 0) == -1) { //access函数是查看是不是存在
			if (mkdir(dir.c_str(), 0777) == -1) {  //如果不存在就用mkdir函数来创建
				WarnL << dir << ":" << get_uv_errmsg();
				return NULL;
			}
		}
	}
	if (path[path.size() - 1] != '/') {
		ret = fopen(file, mode);
	}
	return ret;
}
bool File::createfile_path(const char *file, unsigned int mod) {
	std::string path = file;
	std::string dir;
	int index = 1;
	while (1) {
		index = path.find('/', index) + 1;
		dir = path.substr(0, index);
		if (dir.length() == 0) {
			break;
		}
		if (_access(dir.c_str(), 0) == -1) { //access函数是查看是不是存在
			if (mkdir(dir.c_str(), mod) == -1) {  //如果不存在就用mkdir函数来创建
				WarnL << dir << ":" << get_uv_errmsg();
				return false;
			}
		}
	}
	return true;
}

//判断是否为目录
bool File::is_dir(const char *path) {
	struct stat statbuf;
	if (stat(path, &statbuf) == 0) {
		//lstat返回文件的信息，文件信息存放在stat结构中
		if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
			return true;
		}
#if !defined(_WIN32)
		if (S_ISLNK(statbuf.st_mode)) {
			char realFile[256] = { 0 };
			readlink(path, realFile, sizeof(realFile));
			return File::is_dir(realFile);
		}
#endif  // !defined(_WIN32)
	}
	return false;
}

//判断是否为常规文件
bool File::is_file(const char *path) {
	struct stat statbuf;
	if (stat(path, &statbuf) == 0) {
		if ((statbuf.st_mode & S_IFMT) == S_IFREG) {
			return true;
		}
#if !defined(_WIN32)
		if (S_ISLNK(statbuf.st_mode)) {
			char realFile[256] = { 0 };
			readlink(path, realFile, sizeof(realFile));
			return File::is_file(realFile);
		}
#endif  // !defined(_WIN32)
	}
	return false;
}

//判断是否是特殊目录
bool File::is_special_dir(const char *path) {
	return strcmp(path, ".") == 0 || strcmp(path, "..") == 0;
}

//生成完整的文件路径
void get_file_path(const char *path, const char *file_name, char *file_path) {
	strcpy(file_path, path);
	if (file_path[strlen(file_path) - 1] != '/') {
		strcat(file_path, "/");
	}
	strcat(file_path, file_name);
}
void File::delete_file(const char *path) {
	DIR *dir;
	dirent *dir_info;
	char file_path[PATH_MAX];
	if (is_file(path)) {
		remove(path);
		return;
	}
	if (is_dir(path)) {
		if ((dir = opendir(path)) == NULL) {
			_rmdir(path);
			closedir(dir);
			return;
		}
		while ((dir_info = readdir(dir)) != NULL) {
			if (is_special_dir(dir_info->d_name)) {
				continue;
			}
			get_file_path(path, dir_info->d_name, file_path);
			delete_file(file_path);
		}
		_rmdir(path);
		closedir(dir);
		return;
	}
	_unlink(path);
}

string File::loadFile(const char *path) {
	FILE *fp = fopen(path,"rb");
	if(!fp){
		return "";
	}
	fseek(fp,0,SEEK_END);
	auto len = ftell(fp);
	fseek(fp,0,SEEK_SET);
	string str(len,'\0');
	fread((char *)str.data(),str.size(),1,fp);
    fclose(fp);
	return str;
}

bool File::saveFile(const string &data, const char *path) {
	FILE *fp = fopen(path,"wb");
	if(!fp){
		return false;
	}
	fwrite(data.data(),data.size(),1,fp);
	fclose(fp);
	return true;
}

string File::parentDir(const string &path) {
	auto parent_dir = path;
	if(parent_dir.back() == '/'){
		parent_dir.pop_back();
	}
	auto pos = parent_dir.rfind('/');
	if(pos != string::npos){
		parent_dir = parent_dir.substr(0,pos + 1);
	}
	return std::move(parent_dir);
}

string File::absolutePath(const string &path,const string &currentPath_in,bool canAccessParent) {
	string currentPath = currentPath_in;
	if(!currentPath.empty()){
		//当前目录不为空
		if(currentPath.front() == '.'){
			//如果当前目录是相对路径，那么先转换成绝对路径
			currentPath = absolutePath(currentPath_in,exeDir(),true);
		}
	} else{
		currentPath = exeDir();
	}

	if(path.empty()){
		//相对路径为空，那么返回当前目录
		return currentPath;
	}

	if(currentPath.back() != '/'){
		//确保当前目录最后字节为'/'
		currentPath.push_back('/');
	}

    auto dir_vec = split(path,"/");
	for(auto &dir : dir_vec){
		if(dir.empty() || dir == "."){
			//忽略空或本文件夹
			continue;
		}
		if(dir == ".."){
			//访问上级目录
			if(!canAccessParent && currentPath.size() <= currentPath_in.size()){
				//不能访问根目录之外的目录
				return "";
			}
			currentPath = parentDir(currentPath);
			continue;
		}
		currentPath.append(dir);
		currentPath.append("/");
	}

	if(path.back() != '/' && currentPath.back() == '/'){
		//在路径是文件的情况下，防止转换成目录
		currentPath.pop_back();
	}
	return currentPath;
}

void File::scanDir(const string &path_in, const function<bool(const string &path, bool isDir)> &cb, bool enterSubdirectory) {
	string path = path_in;
	if(path.back() == '/'){
		path.pop_back();
	}

	DIR *pDir;
	dirent *pDirent;
	if ((pDir = opendir(path.data())) == NULL) {
		//文件夹无效
		return;
	}
	while ((pDirent = readdir(pDir)) != NULL) {
		if (is_special_dir(pDirent->d_name)) {
			continue;
		}
		if(pDirent->d_name[0] == '.'){
			//隐藏的文件
			continue;
		}
		string strAbsolutePath = path + "/" + pDirent->d_name;
		bool isDir = is_dir(strAbsolutePath.data());
		if(!cb(strAbsolutePath,isDir)){
			//不再继续扫描
			break;
		}

		if(isDir && enterSubdirectory){
			//如果是文件夹并且扫描子文件夹，那么递归扫描
			scanDir(strAbsolutePath,cb,enterSubdirectory);
		}
	}
	closedir(pDir);
}
#endif

// https://blog.csdn.net/10km/article/details/51005649
// C++11:for_each_file遍历目录处理文件

/* realpath是用来将参数path所指的相对路径转换成绝对路径
 * 如果resolved_path为NULL，则该函数调用malloc分配一块大小为PATH_MAX的内存
 * 来存放解析出来的绝对路径，并返回指向这块区域的指针, 主动free这块内存 */
std::string GetRealPath(const std::string& path) {
  char realPath[256] = {0};
  if (!realpath(path.c_str(), realPath)) {
    // LOG(ERROR) << "Get real path failed";
  }
  return std::string(realPath);
}

/* 如果 size太小无法保存该地址，返回 NULL 并设置 errno 为 ERANGE */
std::string GetCurrentWorkDir() {
  char workPath[256] = {0};
  if (!getcwd(workPath, 0) && errno == ERANGE) {
    LOG(ERROR) << "Get current working directory failed";
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

// Stolen from
// https://gist.github.com/JonathonReinhart/8c0d90191c38af2dcadb102c4e202950
int mkdir_p(const char* path, mode_t mode) {
  /* Adapted from http://stackoverflow.com/a/2336245/119527 */
  const size_t len = strlen(path);
  char _path[PATH_MAX];
  char* p;

  errno = 0;

  /* Copy string so its mutable */
  if (len > sizeof(_path) - 1) {
    errno = ENAMETOOLONG;
    return -1;
  }
  strcpy(_path, path);

  /* Iterate the string */
  for (p = _path + 1; *p; p++) {
    if (*p == '/') {
      /* Temporarily truncate */
      *p = '\0';
      /* check if file exists before mkdir to avoid EACCES */
      if (access(_path, F_OK) != 0) {
        /* fail if error is anything but file does not exist */
        if (errno != ENOENT) {
          return -1;
        }
        if (mkdir(_path, mode) != 0) {
          if (errno != EEXIST) return -1;
        }
      }

      *p = '/';
    }
  }

  if (access(_path, F_OK) != 0) {
    /* fail if error is anything but file does not exist */
    if (errno != ENOENT) {
      return -1;
    }
    if (mkdir(_path, mode) != 0) {
      if (errno != EEXIST) return -1;
    }
  }

  return 0;
}

void temp_file(FILE** fp, std::string& name) {
  char n[] = "/tmp/lightscanXXXXXX";
  int fd = mkstemp(n);
  *fp = fdopen(fd, "wb+");
  name = std::string(n);
}

bool CreateDirWithUid(const std::string& path, const mode_t mode, uid_t user) {
  if (mkdir(path.c_str(), mode) < 0) {
    LOG(ERROR) << "Fail to create dir: " << path;
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
#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
  struct stat status;
  int result = stat(dir.c_str(), &status);
  if (result != 0) {
    if ((errno == ENOENT) || (errno == ENOTDIR)) return false;
  }

  if (S_ISDIR(status.st_mode))
    return true;
  else
    return false;
#elif defined(WIN32) || defined(WIN64)
  DWORD attributes = GetFileAttributesW(wstring().c_str());
  if (attributes == INVALID_FILE_ATTRIBUTES) return false;

  if (attributes & FILE_ATTRIBUTE_DIRECTORY)
    return true;
  else
    return false;
#endif
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
    LOG(ERROR) << "Could not open \"" << folder << "\" directory.";
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
      //     LOG_INFO << "delete /dev/shm/" << de->d_name;
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
    LOG(ERROR) << "cannot own -1";
  }
}

File::File(const char* name, int flags, mode_t mode)
    : fd_(::open(name, flags, mode)), ownsFd_(false) {
  if (fd_ == -1) {
    LOG(ERROR) << "Open file failed";
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
    LOG(ERROR)
        << "closing fd " << fd << ", it may already "
        << "have been closed. Another time, this might close the wrong FD.";
  }
}

File File::temporary() {
  // make a temp file with tmpfile(), dup the fd, then return it in a File.
  FILE* tmpFile = tmpfile();
  if (!tmpFile) {
    LOG(ERROR) << "tmpfile() failed";
  }

  int fd = ::dup(fileno(tmpFile));
  if (fd < 0) {
    LOG(ERROR) << "dup() failed";
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
      LOG(ERROR) << "dup() failed";
    }

    return File(fd, true);
  }

  return File();
}

void File::close() {
  if (!closeNoThrow()) {
    LOG(ERROR) << "close() failed";
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
    LOG(ERROR) << "flock() failed (lock)";
  }
}

bool File::doTryLock(int op) {
  int r = flockNoInt(fd_, op | LOCK_NB);
  // flock returns EWOULDBLOCK if already locked
  if (r == -1 && errno == EWOULDBLOCK) {
    return false;
  }

  if (unlikely(r == -1)) {
    LOG(ERROR) << "flock() failed (try_lock)";
  }
  return true;
}

void File::unlock() {
  if (flockNoInt(fd_, LOCK_UN) < 0) {
    LOG(ERROR) << "flock() failed (unlock)";
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
      LOG(ERROR) << "error: could not read pid file, " << pid_file << ":"
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
  if (::kill(pid, 0) < 0) return 0;
  LOG(ERROR) << "process is already running!  process id: " << (long int)pid;
  return 1;
}

void WritePidFile(const std::string& pid_file) {
  FILE* fp = NULL;
  fp = fopen(pid_file.c_str(), "w+");
  if (!fp) {
    LOG(ERROR) << "could not write pid file =>" << pid_file << strerror(errno);
    return;
  }
  fprintf(fp, "%d\n", getpid());
  fclose(fp);
}

bool ReadFileContents(const std::string& file, std::string* content) {
  int fd = ::open(file.c_str(), O_RDONLY);
  if (fd < 0) {
    return false;
  }
  char buffer[100];
  int32_t size = 0;
  while ((size = ::read(fd, buffer, sizeof(buffer))) > 0) {
    content->append(buffer, size);
  }
  return true;
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

// The type-value for a ZFS FS in fstatfs.
#define FS_ZFS_TYPE 0xde

// On FreeBSD, ZFS fallocate always fails since it is considered impossible to
// reserve space on a COW filesystem. posix_fallocate() returns EINVAL
// Linux in this case already emulates the reservation in glibc
// In which case it is allocated manually, and still that is not a real
// guarantee
// that a full buffer is allocated on disk, since it could be compressed.
// To prevent this the written buffer needs to be loaded with random data.
int ManualFallocate(int fd, off_t offset, off_t len) {
  int r = lseek(fd, offset, SEEK_SET);
  if (r == -1) return errno;
  char data[1024 * 128];
  // TODO: compressing filesystems would require random data
  // FIPS zeroization audit 20191115: this memset is not security related.
  ::memset(data, 0x42, sizeof(data));
  for (off_t off = 0; off < len; off += sizeof(data)) {
    if (off + static_cast<off_t>(sizeof(data)) > len)
      r = ::write(fd, data, len - off);
    else
      r = ::write(fd, data, sizeof(data));
    if (r == -1) {
      return errno;
    }
  }
  return 0;
}

int IsZFS(int basedir_fd) {
#ifndef _WIN32
  struct statfs basefs;
  (void)::fstatfs(basedir_fd, &basefs);
  return (basefs.f_type == FS_ZFS_TYPE);
#else
  return 0;
#endif
}

int PosixFallocate(int fd, off_t offset, off_t len) {
// Return 0 if oke, otherwise errno > 0

#ifdef HAVE_POSIX_FALLOCATE
  if (IsZFS(fd)) {
    return ManualFallocate(fd, offset, len);
  } else {
    return ::posix_fallocate(fd, offset, len);
  }
#elif defined(__APPLE__)
  fstore_t store;
  store.fst_flags = F_ALLOCATECONTIG;
  store.fst_posmode = F_PEOFPOSMODE;
  store.fst_offset = offset;
  store.fst_length = len;

  int ret = fcntl(fd, F_PREALLOCATE, &store);
  if (ret == -1) {
    ret = errno;
  }
  return ret;
#else
  return ManualFallocate(fd, offset, len);
#endif
}

#if defined(WIN32)
int fsync(int fd) {
  HANDLE handle = (HANDLE*)_get_osfhandle(fd);
  if (handle == INVALID_HANDLE_VALUE) return -1;
  if (!FlushFileBuffers(handle)) return -1;
  return 0;
}
ssize_t pwrite(int fd, const void* buf, size_t count, off_t offset) {
  DWORD bytes_written = 0;

  HANDLE handle = (HANDLE*)_get_osfhandle(fd);
  if (handle == INVALID_HANDLE_VALUE) return -1;

  OVERLAPPED overlapped = {0};
  ULARGE_INTEGER offsetUnion;
  offsetUnion.QuadPart = offset;

  overlapped.Offset = offsetUnion.LowPart;
  overlapped.OffsetHigh = offsetUnion.HighPart;

  if (!::WriteFile(handle, buf, count, &bytes_written, &overlapped))
    // we may consider mapping error codes, although that may
    // not be exhaustive.
    return -1;

  return bytes_written;
}
ssize_t pread(int fd, void* buf, size_t count, off_t offset) {
  DWORD bytes_read = 0;

  HANDLE handle = (HANDLE*)_get_osfhandle(fd);
  if (handle == INVALID_HANDLE_VALUE) return -1;

  OVERLAPPED overlapped = {0};
  ULARGE_INTEGER offsetUnion;
  offsetUnion.QuadPart = offset;

  overlapped.Offset = offsetUnion.LowPart;
  overlapped.OffsetHigh = offsetUnion.HighPart;

  if (!ReadFile(handle, buf, count, &bytes_read, &overlapped)) {
    if (GetLastError() != ERROR_HANDLE_EOF) return -1;
  }

  return bytes_read;
}
ssize_t preadv(int fd, const struct iovec* iov, int iov_cnt) {
  ssize_t read = 0;

  for (int i = 0; i < iov_cnt; i++) {
    int r = ::read(fd, iov[i].iov_base, iov[i].iov_len);
    if (r < 0) return r;
    read += r;
    if (r < iov[i].iov_len) break;
  }

  return read;
}

ssize_t writev(int fd, const struct iovec* iov, int iov_cnt) {
  ssize_t written = 0;

  for (int i = 0; i < iov_cnt; i++) {
    int r = ::write(fd, iov[i].iov_base, iov[i].iov_len);
    if (r < 0) return r;
    written += r;
    if (r < iov[i].iov_len) break;
  }

  return written;
}

#ifndef WIN32
int get_fs_stats(ceph_data_stats_t& stats, const char* path) {
  if (!path) return -EINVAL;

  struct statfs stbuf;
  int err = ::statfs(path, &stbuf);
  if (err < 0) {
    return -errno;
  }

  stats.byte_total = stbuf.f_blocks * stbuf.f_bsize;
  stats.byte_used = (stbuf.f_blocks - stbuf.f_bfree) * stbuf.f_bsize;
  stats.byte_avail = stbuf.f_bavail * stbuf.f_bsize;
  stats.avail_percent = (((float)stats.byte_avail / stats.byte_total) * 100);
  return 0;
}
#else
int get_fs_stats(ceph_data_stats_t& stats, const char* path) {
  ULARGE_INTEGER avail_bytes, total_bytes, total_free_bytes;

  if (!GetDiskFreeSpaceExA(path, &avail_bytes, &total_bytes,
                           &total_free_bytes)) {
    return -EINVAL;
  }

  stats.byte_total = total_bytes.QuadPart;
  stats.byte_used = total_bytes.QuadPart - total_free_bytes.QuadPart;
  // may not be equal to total_free_bytes due to quotas
  stats.byte_avail = avail_bytes.QuadPart;
  stats.avail_percent = ((float)stats.byte_avail / stats.byte_total) * 100;
  return 0;
}
#endif

/***/
FILE* fopen(filename_t const& filename, std::string const& mode) {
  FILE* fp{nullptr};
#if defined(_WIN32)
  std::wstring const w_mode = s2ws(mode);
  fp = ::_wfsopen((filename.c_str()), w_mode.data(), _SH_DENYNO);
#else
  fp = ::fopen(filename.data(), mode.data());
#endif
  if (!fp) {
    std::ostringstream error_msg;
    error_msg << "fopen failed with error message errno: \"" << errno << "\"";
    QUILL_THROW(QuillError{error_msg.str()});
  }
  return fp;
}

#if 0
/***/
size_t fsize(FILE* file)
{
  if (!file)
  {
    QUILL_THROW(QuillError{"fsize failed. file is nullptr"});
  }

#if defined(_WIN32) && !defined(__CYGWIN__)
  auto const fd = ::_fileno(file);
  auto const ret = ::_filelength(fd);

  if (ret >= 0)
  {
    return static_cast<size_t>(ret);
  }
#else
  auto const fd = fileno(file);
  struct stat st;

  if (fstat(fd, &st) == 0)
  {
    return static_cast<size_t>(st.st_size);
  }
#endif

  // failed to get the file size
  std::ostringstream error_msg;
  error_msg << "fopen failed with error message errno: \"" << errno << "\"";
  QUILL_THROW(QuillError{error_msg.str()});
}
/***/
int remove(std::string const& filename) noexcept
{
#if defined(_WIN32)
  return ::_wremove(filename.c_str());
#else
  return std::remove(filename.c_str());
#endif
}

/***/
void rename(filename_t const& previous_file, filename_t const& new_file)
{
#if defined(_WIN32)
  int const res = ::_wrename(previous_file.c_str(), new_file.c_str());
#else
  int const res = std::rename(previous_file.c_str(), new_file.c_str());
#endif

  if (QUILL_UNLIKELY(res != 0))
  {
    std::ostringstream error_msg;
    error_msg << "failed to rename previous log file during rotation, with error message errno: \""
              << errno << "\"";
    QUILL_THROW(QuillError{error_msg.str()});
  }
}

#ifdef _WIN32
typedef SYSTEMTIME TIME_TYPE;
#else  // UNIX, OSX
typedef struct tm TIME_TYPE;
#endif


void DiagnosticFilename::LocalTime(TIME_TYPE* tm_struct) {
#ifdef _WIN32
  GetLocalTime(tm_struct);
#else  // UNIX, OSX
  struct timeval time_val;
  gettimeofday(&time_val, nullptr);
  localtime_r(&time_val.tv_sec, tm_struct);
#endif
}

// Defined in node_internals.h
std::string DiagnosticFilename::MakeFilename(
    uint64_t thread_id,
    const char* prefix,
    const char* ext) {
  std::ostringstream oss;
  TIME_TYPE tm_struct;
  LocalTime(&tm_struct);
  oss << prefix;
#ifdef _WIN32
  oss << "." << std::setfill('0') << std::setw(4) << tm_struct.wYear;
  oss << std::setfill('0') << std::setw(2) << tm_struct.wMonth;
  oss << std::setfill('0') << std::setw(2) << tm_struct.wDay;
  oss << "." << std::setfill('0') << std::setw(2) << tm_struct.wHour;
  oss << std::setfill('0') << std::setw(2) << tm_struct.wMinute;
  oss << std::setfill('0') << std::setw(2) << tm_struct.wSecond;
#else  // UNIX, OSX
  oss << "."
            << std::setfill('0')
            << std::setw(4)
            << tm_struct.tm_year + 1900;
  oss << std::setfill('0')
            << std::setw(2)
            << tm_struct.tm_mon + 1;
  oss << std::setfill('0')
            << std::setw(2)
            << tm_struct.tm_mday;
  oss << "."
            << std::setfill('0')
            << std::setw(2)
            << tm_struct.tm_hour;
  oss << std::setfill('0')
            << std::setw(2)
            << tm_struct.tm_min;
  oss << std::setfill('0')
            << std::setw(2)
            << tm_struct.tm_sec;
#endif
  oss << "." << getpid();
  oss << "." << thread_id;
  oss << "." << std::setfill('0') << std::setw(3) << ++seq;
  oss << "." << ext;
  return oss.str();
}

#endif

#endif

}  // namespace MSF