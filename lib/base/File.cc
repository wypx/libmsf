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
	sprintf(namebuf, "%s\\*.*", name);

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