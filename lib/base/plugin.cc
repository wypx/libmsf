#include "plugin.h"

#include <butil/logging.h>
#include <sys/syscall.h>

#include "file.h"

using namespace MSF;

namespace MSF {

int pluginFuncStub(const char* func, ...) {
  LOG(ERROR) << "Func not found here.";
  return -404;
}

PluginManager::~PluginManager() { unloadAll(); }

bool PluginManager::load(const std::string& pluginPath) {
  std::string plugName = getPluginName(pluginPath);
  std::string realPath = resolvePathExtension(pluginPath);

  int iRet = IsFileExist(pluginPath);
  if (iRet < 0) {
    LOG(ERROR) << "Specified plugin not existing: " << pluginPath;
    return false;
  }
  MSF_DLHANDLE handle = MSF_DLOPEN_N(realPath.c_str());
  if (!handle) {
    LOG(ERROR) << "Fail to open plugin :" << pluginPath
               << " error:" << MSF_DLERROR();
    return false;
  }

  MSF_DLERROR(); /* Clear any existing error */

  /* Writing: cosine = (double (*)(double)) dlsym(handle, "cos");
     would seem more natural, but the C99 standard leaves
     casting from "void *" to a function pointer undefined.
     The assignment used below is the POSIX.1-2003 (Technical
     Corrigendum 1) workaround; see the Rationale for the
     POSIX specification of dlsym(). */
  Plugin* plug = (Plugin*)MSF_DLSYM(handle, plugName.c_str());
  if (!plug) {
    LOG(ERROR) << "Fail to load plugin :" << pluginPath
               << "error:" << MSF_DLERROR();
    return false;
  }

  plug->_handle = handle;

  //查重
  // plugins[plugName] = std::shared_ptr(MSF_PLUGIN)(plugin);
  _plugins[plugName] = plug;

  return true;
}

bool PluginManager::load(const std::string& folder,
                         const std::string& pluginName) {
  if (folder.empty()) {
    return load(pluginName);
  } else if (folder[folder.size() - 1] == '/' ||
             folder[folder.size() - 1] == '\\') {
    return load(folder + pluginName);
  }
  return load(folder + '/' + pluginName);
}

int PluginManager::loadFromFolder(const std::string& folder, bool recursive) {
  std::list<std::string> files;
  ListFiles(files, folder, MSF_LIB_EXTENSION, recursive);
  // try to load every library
  int res = 0;
  std::list<std::string>::const_iterator it;
  for (it = files.begin(); it != files.end(); ++it) {
    if (load(*it)) ++res;
  }
  return res;
}

bool PluginManager::unload(const std::string& pluginName) {
  std::string plugName = getPluginName(pluginName);
  std::map<std::string, Plugin*>::iterator it = _plugins.find(plugName);
  if (it != _plugins.end()) {
    MSF_DLCLOSE(it->second->_handle);
    _plugins.erase(it);
    return true;
  }
  return false;
}

void PluginManager::unloadAll() {
  std::map<std::string, Plugin*>::iterator it;
  for (it = _plugins.begin(); it != _plugins.end(); ++it) {
    MSF_DLCLOSE(it->second->_handle);
  }
  _plugins.clear();
}

std::string PluginManager::resolvePathExtension(const std::string& path) {
  size_t lastDash = path.find_last_of("/\\");
  size_t lastDot = path.find_last_of('.');
  if (lastDash == std::string::npos)
    lastDash = 0;
  else
    ++lastDash;
  if (lastDot < lastDash || lastDot == std::string::npos) {
    // path without extension, add it
    return path + "." + MSF_LIB_EXTENSION;
  }
  return path;
}

std::string PluginManager::getPluginName(const std::string& path) {
  size_t lastDash = path.find_last_of("/\\");
  size_t lastDot = path.find_last_of('.');
  if (lastDash == std::string::npos)
    lastDash = 0;
  else
    ++lastDash;
  if (lastDot < lastDash || lastDot == std::string::npos) {
    // path without extension
    lastDot = path.length();
  }
  return path.substr(lastDash, lastDot - lastDash);
}

int PluginManager::driverInsmod(const std::string& ko_path) {
  int fd = -1;
  struct stat st;
  uint32_t len;
  void* map = NULL;
  int rc = -1;
  char option[2] = {0};

  fd = ::open(ko_path.c_str(), O_RDONLY | O_TRUNC, 0644);
  if (fd < 0) {
    LOG(ERROR) << "Fail to open " << ko_path;
    return -1;
  }

  rc = fstat(fd, &st);
  if (rc < 0) {
    close(fd);
    LOG(ERROR) << "Fail to lstat " << ko_path;
    return -1;
  }

  len = st.st_size;
  map = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
  if (map == MAP_FAILED) {
    close(fd);
    LOG(ERROR) << "Fail to map" << ko_path;
    return -1;
  }

  strcpy(option, " ");
  rc = syscall(__NR_init_module, map, len, option);
  if (rc != 0) {
    munmap(map, len);
    close(fd);
    LOG(ERROR) << "Cannot insert " << ko_path;
    return -1;
  }
  munmap(map, len);
  close(fd);
  return 0;
}

int PluginManager::driverRmmod(const std::string& ko_path, const int mode) {
  int flags = O_NONBLOCK | O_EXCL;
  int lRet;
  // char szName[128] = {0};
  int iFileLen = 0;

  if (ko_path.empty()) {
    return -1;
  }

  iFileLen = ko_path.length();
  if (iFileLen >= 128 || iFileLen == 0) {
    LOG(ERROR) << "filename length error: ." << iFileLen;
    return -1;
  }

  // --wait
  if ((mode & MSF_RMMOD_MODE_WAIT)) {
    flags &= ~O_NONBLOCK;
  }
  // --force
  if ((mode & MSF_RMMOD_MODE_FORCE)) {
    flags |= O_TRUNC;
  }

  lRet = syscall(__NR_delete_module, ko_path, flags);
  if (lRet != 0) {
    LOG(ERROR) << "Cannot rmmod " << ko_path;
    return -1;
  }
  return 0;
}

/*
这里有一个函数名字是 foo.当我们同时使用test1.o和test2.o,
如果引用符号foo,最终使用到的是 func.c中定义的函数,
而不是 __foo, 虽然它有一个别名 foo.也就是说,
我们最终使用到的函数会是"实际做事"的那个函数
当然, 单独使用 dummy.o 链接的话使用的是那个“不做事”的函数.
如果test2.o 中的 foo 不是 weak symbol 的话,在链接时会产生冲突,这就是我们要使用
weak 的原因。

glibc 的实现里面经常用 weak alias,比如它的 socket 函数，
在 C 文件里面你会看到一个 __socket
函数，它几乎什么都没有做,只是设置了一些错误代码,返回些东西而已. 在同一个 C
文件里面会再声明一个 __socket 的 weak alias 别名 socket.
实际完成工作的代码通过汇编来实现，在另外的汇编文件里面会有设置系统调用号,
执行 sysenter 或者 int 等动作去请求系统调用.
以前看 glibc 里面系统调用的实现的时候郁闷过很久,就是那个时候才知道了 weak alias
这个东西。

看来weak修饰符的原本含义是让weak弱类型的函数可以被其它同名函数覆盖（即不会发生冲突），
如果没有其它同名函数，就使用该weak函数，类似于是默认函数；
但是，受到“库”这一特别存在的影响，weak弱类型对库变得“不太可靠”了，
而且根据Weak definitions aren’t so
weak来看，对于新版本的glibc，weak修饰符仅针对静态库产生影响。
weakref：可以说是weak与alias的结合版本（见上面英文引用），但在当前，被weakref修饰的符号必须是static的。
http://gcc.gnu.org/onlinedocs/gcc-4.4.0/gcc/Function-Attributes.html
Weak Alias 是 gcc 扩展里的东西，实际上是函数的属性
这个东西在库的实现里面可能会经常用到，比如 glibc 里面就用了不少。
抄录一段 gcc 手册里面的话解释下函数属性是干啥的
In GNU C, you declare certain things about functions called in your program
which help the compiler optimize function calls and check your code more
carefully.
*/

int __foo() {
  printf("I do no thing1.\n");
  return 0;
}
// int foo() __attribute__ ((weak, alias("__foo")));
//等效  asm(".weak foo\n\t .set symbol1, __foo\n\t");

/*
    weak 和 alias 分别是两个属性。
    weak 使得 foo 这个符号在目标文件中作为 weak symbol 而不是 global symbol。
    用 nm 命令查看编译 dummy.c 生成的目标文件可用看到 foo 是一个 weak symbol，
    它前面的标记是
   W。给函数加上weak属性时,即使函数没定义，函数被调用也可以编译成功。 00000000 T
   __foo 00000000 W foo U puts

    而 alias 则使 foo 是 __foo 的一个别名,__foo 和 foo
   必须在同一个编译单元中定义,否则会编译出错。

    /总结weak属性
    //（1）asm(".weak symbol1\n\t .set symbol1, symbol222\n\t");与
    //     void symbol1() __attribute__ ((weak,alias("symbol222")));等效。
    //（2）给函数加上weak属性时，即使函数没定义，函数被调用也可以编译成功。
    //（3）当有两个函数同名时，则使用强符号（也叫全局符号,即没有加weak的函数）来代替弱符号（加weak的函数）。
    //（4）当函数没有定义，但如果是“某个函数”的别名时，如果该函数被调用，就间接调用“某个函数”。

    */
// https://www.cnblogs.com/fly-narrow/p/4629253.html
}  // namespace MSF