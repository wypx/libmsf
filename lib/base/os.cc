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
#include "os.h"

#include <butil/logging.h>

#include <cxxabi.h>
#include <execinfo.h>
#include <fcntl.h>
#include <numaif.h>
#include <sys/mman.h>
#include <sys/stat.h>
// #include <grp.h>
#include <sys/reboot.h>
#include <sys/sysinfo.h>

#include <algorithm>
#include <fstream>
#include <thread>
#include <regex>

#include "affinity.h"
#include "define.h"

using namespace MSF;

// https://www.cnblogs.com/lidabo/p/7554473.html

namespace MSF {

OsInfo::OsInfo() { osInit(); }
OsInfo::~OsInfo() {}

uint32_t OsInfo::getSuggestThreadNum() {
  uint32_t n = std::thread::hardware_concurrency();
  LOG(TRACE) << n << " concurrent threads are supported.";
  return n;
}

bool OsInfo::setUser(uid_t user, const std::string& userName) {
  char* group = NULL;
  struct passwd* pwd = NULL;
  struct group* grp = NULL;

  if (user != (uid_t)MSF_CONF_UNSET_UINT) {
    LOG(TRACE) << "User is duplicate.";
    return true;
  }

  if (geteuid() == 0) {
    gid_t groupId = 0;
    if (setgid(groupId) == -1) {
      /* fatal */
      exit(2);
    }

    if (initgroups(userName.c_str(), groupId) == -1) {
      exit(2);
    }

    if (setuid(user) == -1) {
      /* fatal */
      exit(2);
    }

    ////
    pwd = getpwnam(userName.c_str());
    if (pwd == nullptr) {
      return false;
    }

    // uid_t user1 = pwd->pw_uid;

    grp = getgrnam(group);
    if (grp == nullptr) {
      return false;
    }
    // gid_t group1 = grp->gr_gid;
    return true;
  } else {
    LOG(TRACE) << "The \"user\" directive makes sense only "
                  "if the master process runs "
                  "with super-user privileges, ignored.";
    return true;
  }
}

// https://blog.csdn.net/zhjutao/article/details/8652252
// https://www.cnblogs.com/cl1024cl/p/6205119.html
/* we don't want our active sessions to be paged out... */
bool OsInfo::disablePageOut() {
  if (mlockall(MCL_CURRENT | MCL_FUTURE)) {
    LOG(ERROR) << "failed to mlockall, diable page out.";
    return false;
  }
  return true;
}

/* oom-killer will not kill us at the night... */
bool OsInfo::oomAdjust() {
  int fd;
  char path[48];
  struct stat statb;

  errno = 0;
  if (nice(-10) == -1 && errno != 0) {
    LOG(ERROR) << "Could not increase process priority:" << strerror(errno);
    return false;
  }

  snprintf(path, 48, "/proc/%d/oom_score_adj", getpid());
  if (stat(path, &statb)) {
    /* older kernel so use old oom_adj file */
    snprintf(path, 48, "/proc/%d/oom_adj", getpid());
  }
  fd = open(path, O_WRONLY);
  if (fd < 0) {
    return false;
  }
  if (write(fd, "-16", 3) < 0) {
    /* for 2.6.11 */
    LOG(ERROR) << "Could not set oom score to -16: " << strerror(errno);
  }
  if (write(fd, "-17", 3) < 0) {
    /* for Andrea's patch */
    LOG(ERROR) << "Could not set oom score to -17: " << strerror(errno);
  }
  close(fd);
  return true;
}

uint32_t OsInfo::getMaxOpenFds() {
  uint32_t maxFds = 0;
  struct rlimit rl;
  /* But if possible, get the actual highest FD we can possibly ever see. */
  if (0 == getrlimit(RLIMIT_NOFILE, &rl)) {
    maxFds = rl.rlim_max;
  } else {
    LOG(ERROR) << "Failed to query maximum file descriptor; "
                  "falling back to maxconns.";
  }
  return maxFds;
}

bool OsInfo::setMaxOpenFds(const uint64_t maxOpenFds) {
  /* We're unlikely to see an FD much higher than maxconns. */
  int nextFd = dup(1);
  uint32_t headRoom = 10; /* account for extra unexpected open FDs */
  uint64_t maxFds = maxOpenFds + headRoom + nextFd;

  struct rlimit rlmt;
  rlmt.rlim_cur = (rlim_t)maxFds;
  rlmt.rlim_max = (rlim_t)maxFds;

  /* next_fd used to count */
  close(nextFd);

  // RLIMIT_NOFILE指定此进程可打开的最大文件描述词大一的值,超出此值,将会产生EMFILE错误。
  if (setrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
    LOG(ERROR) << "Set rlimit nofile faild, errno: " << errno;
    return false;
  }
  return true;
}

/* allow coredump after setuid() in Linux 2.4.x */
bool OsInfo::EnableCoredump() {
#if HAVE_PRCTL&& defined(PR_SET_DUMPABLE)
  /* Set Linux DUMPABLE flag */
  if (prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) != 0) {
    LOG(ERROR) << "prctl: " << strerror(errno);
    return false;
  }
#endif

  /* Make sure coredumps are not limited */
  struct rlimit rlim;
  if (getrlimit(RLIMIT_CORE, &rlim) == 0) {
    LOG(TRACE) << "Coredump curr: " << rlim.rlim_cur
               << " max is: " << rlim.rlim_max;
    rlim.rlim_cur = rlim.rlim_max;
    if (setrlimit(RLIMIT_CORE, &rlim) < 0) {
      LOG(ERROR) << "Enable coredump faild:" << strerror(errno);
      return false;
    }
  }
  return true;
}

// https://blog.csdn.net/u011417820/article/details/71435031
// https://blog.csdn.net/cp3alai/article/details/93968796
// https://blog.csdn.net/fengxinze/article/details/6800175
/**
 * ulimit - c 1024
 * */
bool OsInfo::setCoreDumpSize(const uint64_t maxCoreSize) {
  if (maxCoreSize == 0) {
    return false;
  }
  struct rlimit rlmt;
  rlmt.rlim_cur = (rlim_t)maxCoreSize;
  rlmt.rlim_max = (rlim_t)maxCoreSize;
  //修改工作进程的core文件尺寸的最大值限制(RLIMIT_CORE),用于在不重启主进程的情况下增大该限制
  if (setrlimit(RLIMIT_CORE, &rlmt) == -1) {
    LOG(ERROR) << "Set rlimit core faild, errno: " << errno;
    return false;
  }
  return true;
}

/**
 * 1. echo "1" > /proc/sys/kernel/core_uses_pid
 * 2. sysctl -w kernel.core_uses_pid=1 kernel.core_uses_pid = 1
 * */
bool OsInfo::setCoreUsePid() {
  std::string pattern = "echo \"1\" > /proc/sys/kernel/core_uses_pid";
  return true;
}

/**
 * 1. echo "/root/core-%h-%s-%e-%p-%t" > /proc/sys/kernel/core_pattern
 * 2. sysctl -w kernel.core_pattern=/home/core/core.%h.%s.%e.%p.%s.%t
 * 以下是参数列表:
 * %p - insert pid into filename 添加pid(进程id)
 * %u - insert current uid into filename 添加当前uid(用户id)
 * %g - insert current gid into filename 添加当前gid(用户组id)
 * %s - insert signal that caused the coredump into the filename
 * 添加导致产生core的信号 %t - insert UNIX time that the coredump occurred into
 * filename 添加core文件生成时的unix时间 %h - insert hostname where the coredump
 * happened into filename 添加主机名 %e - insert coredumping executable name
 * into filename 添加导致产生core的命令名
 * */
bool OsInfo::setCorePath(const std::string& path) {
  std::string cmdCorePath =
      "echo \"/home/core/core-%h-%s-%e-%p-%t\" > /proc/sys/kernel/core_pattern";
  return ::system(cmdCorePath.c_str()) == 0 ? true : false;
}

void run_as_user(const char* username, char* argv[]) {
  if (::geteuid() != 0) return; /* ignore if current user is not root */

  const struct passwd* userinfo = ::getpwnam(username);
  if (!userinfo) {
    // LOGERR("[run_as_user] user:'%s' does not exist in this system",
    // username);
    return;
  }

  if (userinfo->pw_uid == 0) return; /* ignore if target user is root */

  if (::setgid(userinfo->pw_gid) < 0) {
    // LOGERR("[run_as_user] change to gid:%u of user:'%s': %s",
    // userinfo->pw_gid, userinfo->pw_name, strerror(errno));
    ::exit(errno);
  }

  // initgroups()用来从组文件(/etc/group)中读取一项组数据,
  // 若该组数据的成员中有参数user 时,便将参数group 组识别码加入到此数据中
  if (::initgroups(userinfo->pw_name, userinfo->pw_gid) < 0) {
    // LOGERR("[run_as_user] initgroups(%u) of user:'%s': %s", userinfo->pw_gid,
    // userinfo->pw_name, strerror(errno));
    ::exit(errno);
  }

  if (::setuid(userinfo->pw_uid) < 0) {
    // LOGERR("[run_as_user] change to uid:%u of user:'%s': %s",
    // userinfo->pw_uid, userinfo->pw_name, strerror(errno));
    ::exit(errno);
  }

  static char execfile_abspath[PATH_MAX] = {0};
  if (::readlink("/proc/self/exe", execfile_abspath, PATH_MAX - 1) < 0) {
    // LOGERR("[run_as_user] readlink('/proc/self/exe'): %s", strerror(errno));
    ::exit(errno);
  }

  if (execv(execfile_abspath, argv) < 0) {
    // LOGERR("[run_as_user] execv('%s', args): %s", execfile_abspath,
    // strerror(errno));
    ::exit(errno);
  }
}

void PrintBuffer(void* pBuff, unsigned int nLen) {
  if (NULL == pBuff || 0 == nLen) {
    return;
  }

  const int nBytePerLine = 16;
  unsigned char* p = (unsigned char*)pBuff;
  char szHex[3 * nBytePerLine + 1] = {0};

  printf("-----------------begin-------------------\n");
  for (unsigned int i = 0; i < nLen; ++i) {
    int idx = 3 * (i % nBytePerLine);
    if (0 == idx) {
      memset(szHex, 0, sizeof(szHex));
    }
#ifdef WIN32
    sprintf_s(&szHex[idx], 4, "%02x ", p[i]);  // buff长度要多传入1个字节
#else
    snprintf(&szHex[idx], 4, "%02x ", p[i]);  // buff长度要多传入1个字节
#endif

    // 以16个字节为一行，进行打印
    if (0 == ((i + 1) % nBytePerLine)) {
      printf("%s\n", szHex);
    }
  }

  // 打印最后一行未满16个字节的内容
  if (0 != (nLen % nBytePerLine)) {
    printf("%s\n", szHex);
  }

  printf("------------------end-------------------\n");
}

/* Display a buffer into a HEXA formated output */
void OsInfo::dumpBuffer(const char* buff, size_t count, FILE* fp) {
  size_t i, j, c;
  bool printnext = true;

  if (count % 16)
    c = count + (16 - count % 16);
  else
    c = count;

  for (i = 0; i < c; i++) {
    if (printnext) {
      printnext = false;
      fprintf(fp, "%.4zu ", i & 0xffff);
    }
    if (i < count)
      fprintf(fp, "%3.2x", buff[i] & 0xff);
    else
      fprintf(fp, "   ");

    if (!((i + 1) % 8)) {
      if ((i + 1) % 16) {
        fprintf(fp, " -");
      } else {
        fprintf(fp, "   ");
        for (j = i - 15; j <= i; j++) {
          if (j < count) {
            if ((buff[j] & 0xff) >= 0x20 && (buff[j] & 0xff) <= 0x7e) {
              fprintf(fp, "%c", buff[j] & 0xff);
            } else {
              fprintf(fp, ".");
            }
          } else {
            fprintf(fp, " ");
          }
        }
        fprintf(fp, "\n");
        printnext = true;
      }
    }
  }
}

/* Default: disabled -- recommended 0.8, 0.9 */
static double warning = 0.0;
static double critical = 0.0;

static int above_watermark(double avg, struct sysinfo* si) {
  /* Expect loadavg to be out of wack first five mins after boot. */
  if (si->uptime < 300) return 0;

  /* High watermark alert disabled */
  if (critical == 0.0) return 0;

  if (avg <= critical) return 0;

  return 1;
}

// https://bbs.csdn.net/topics/300192682
void OsInfo::GetLoadAverage() {
  double avg, load[3];
  struct sysinfo si;

  if (::sysinfo(&si)) {
    // ERROR("Failed reading system loadavg");
    return;
  }

  /** resource usage */
  struct rusage ru;
  if (::getrusage(RUSAGE_CHILDREN, &ru) == 0) {
  }

  for (int i = 0; i < 3; i++) {
    load[i] = (double)si.loads[i] / (1 << SI_LOAD_SHIFT);
  }
  // LOG(INFO) << "Loadavg: %.2f, %.2f, %.2f (1, 5, 15 min)", load[0], load[1],
  // load[2];

  avg = (load[0] + load[1]) / 2.0;
  // DEBUG("System load: %.2f, %.2f, %.2f (1, 5, 15 min), avg: %.2f (1 + 5),
  // warning: %.2f, reboot: %.2f",  load[0], load[1], load[2], avg, warning,
  // critical);

  if (avg > warning) {
    if (above_watermark(avg, &si)) {
      // EMERG("System load too high, %.2f > %0.2f, rebooting system ...", avg,
      // critical);
      // if (checker_exec(exec, "loadavg", 1, avg, warning, critical)) {
      //   // wdt_forced_reset(w->ctx, getpid(), PACKAGE ":loadavg", 0);
    }
    return;

    // WARN("System load average very high, %.2f > %0.2f!", avg, warning);
    // checker_exec(exec, "loadavg", 0, avg, warning, critical);
  }
}

// https://blog.csdn.net/feizhijiang/article/details/36187909
// https://www.cnblogs.com/cobbliu/articles/9239710.html
bool OsInfo::Reboot() {
  // 同步磁盘数据,将缓存数据回写到硬盘,以防数据丢失
  ::sync();
  return ::reboot(RB_AUTOBOOT) == 0;
}

bool OsInfo::set_hostname_persist(const std::string& hostname) {
  hostname_ = hostname;
  std::string cmd = "sudo hostnamectl set-hostname " + hostname;
  ::system(cmd.c_str());
  return Reboot();
}

// https://www.cnblogs.com/mickole/p/3246702.html
void OsInfo::writeStackTrace(const std::string& filePath) {
  int fd = ::open(filePath.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644);
  void* buffer[100];
  int nptrs;

  nptrs = backtrace(buffer, 100);
  backtrace_symbols_fd(buffer, nptrs, fd);
  if (::write(fd, "\n", 1) != 1) {
    /* We don't care, but this stops a warning on Ubuntu */
  }
  ::close(fd);
}

struct Frame {
  void* address;         //!< Frame address
  std::string module;    //!< Frame module
  std::string function;  //!< Frame function
  std::string filename;  //!< Frame file name
  int line;              //!< Frame line number

  //! Get string from the current stack trace frame
  std::string string() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
  }

  //! Output stack trace frame into the given output stream
  friend std::ostream& operator<<(std::ostream& os, const Frame& frame);
};
std::vector<Frame> _frames;

// 认识ptrace函数
// https://www.cnblogs.com/heixiang/p/10988992.html
// https://www.cnblogs.com/axiong/p/6184638.html?utm_source=itdadao&utm_medium=referral
const std::string OsInfo::stackTrace(bool demangle) {
#if (defined(unix) || defined(__unix) || defined(__unix__) || \
     defined(__APPLE__)) &&                                   \
    !defined(__CYGWIN__)
  // get void*'s for all entries on the stack
  std::string stack;
  const int max_frames = 200;
  void* frame[max_frames];
  int nptrs = ::backtrace(frame, max_frames);

  // Print out all frames to stderr. Separate sigsegvHandler stack
  // [dumpBacktrace][sigsegvHandler][libc throwing] from actual
  // code stack.
  char** strings = ::backtrace_symbols(frame, nptrs);
  if (strings) {
    size_t len = 256;
    char* demangled = demangle ? static_cast<char*>(::malloc(len)) : nullptr;
    for (int i = 1; i < nptrs;
         ++i)  // skipping the 0-th, which is this function
    {
      if (demangle) {
        // https://www.cnblogs.com/BloodAndBone/p/7912179.html
        // https://panthema.net/2008/0901-stacktrace-demangled/
        // bin/exception_test(_ZN3Bar4testEv+0x79) [0x401909]
        char* left_par = nullptr;
        char* plus = nullptr;
        for (char* p = strings[i]; *p; ++p) {
          if (*p == '(')
            left_par = p;
          else if (*p == '+')
            plus = p;
        }

        if (left_par && plus) {
          *plus = '\0';
          int status = 0;
          char* ret =
              abi::__cxa_demangle(left_par + 1, demangled, &len, &status);
          *plus = '+';
          if (status == 0) {
            demangled = ret;  // ret could be realloc()
            stack.append(strings[i], left_par + 1);
            stack.append(demangled);
            stack.append(plus);
            stack.push_back('\n');
            continue;
          }
        }
      }
      // Fallback to mangled names
      stack.append(strings[i]);
      stack.push_back('\n');
    }
    free(demangled);
    free(strings);
  }
  return stack;
#elif defined(WIN32) || defined(WIN64)
  const int capacity = 1024;
  void* frames[capacity];

  // Capture the current stack trace
  USHORT captured = CaptureStackBackTrace(skip + 1, capacity, frames, nullptr);

  // Resize stack trace frames vector
  _frames.resize(captured);

  // Capture stack trace snapshot under the critical section
  static CriticalSection cs;
  Locker<CriticalSection> locker(cs);

  // Fill all captured frames with symbol information
  for (int i = 0; i < captured; ++i) {
    auto& frame = _frames[i];

    // Get the frame address
    frame.address = frames[i];

#if defined(DBGHELP_SUPPORT)
    // Get the current process handle
    HANDLE hProcess = GetCurrentProcess();

    // Get the frame module
    IMAGEHLP_MODULE64 module;
    ZeroMemory(&module, sizeof(module));
    module.SizeOfStruct = sizeof(module);
    if (SymGetModuleInfo64(hProcess, (DWORD64)frame.address, &module)) {
      const char* image = std::strrchr(module.ImageName, '\\');
      if (image != nullptr) frame.module = image + 1;
    }

    // Get the frame function
    char symbol[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
    ZeroMemory(&symbol, countof(symbol));
    PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)symbol;
    pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    pSymbol->MaxNameLen = MAX_SYM_NAME;
    if (SymFromAddr(hProcess, (DWORD64)frame.address, nullptr, pSymbol)) {
      char buffer[4096];
      if (UnDecorateSymbolName(pSymbol->Name, buffer, (DWORD)countof(buffer),
                               UNDNAME_NAME_ONLY) > 0)
        frame.function = buffer;
    }

    // Get the frame file name and line number
    DWORD offset = 0;
    IMAGEHLP_LINE64 line;
    ZeroMemory(&line, sizeof(line));
    line.SizeOfStruct = sizeof(line);
    if (SymGetLineFromAddr64(hProcess, (DWORD64)frame.address, &offset,
                             &line)) {
      if (line.FileName != nullptr) frame.filename = line.FileName;
      frame.line = line.LineNumber;
    }
#endif
  }
#endif
}

/* Numa TRACE*/
void OsInfo::vnodeInit(void) {
#if 0
    int rc;

     en_numa = numa_available();
    if ( en_numa < 0) {
        LOG(ERROR) << "Your system does not support NUMA API.";
        return;
    }

     numacnt = numa_max_node();

    LOG(ERROR) << "System numa en:" <<  en_numa;
    LOG(ERROR) << "System numa num:" <<  numacnt;

    int nd;
    char *man = (char *)numa_alloc(1000);
    *man = 1;
    if (get_mempolicy(&nd, NULL, 0, man, MPOL_F_NODE|MPOL_F_ADDR) < 0)
        perror("get_mempolicy");
    else
        LOG(ERROR) << "my node is : " << nd;

    numa_free(man, 1000);
#endif
}

bool OsInfo::sysInit() {
  struct rlimit rlmt;
  struct utsname u;

  if (uname(&u) == -1) {
    LOG(ERROR) << "Uname failed, errno:" << errno;
    return false;
  }

  sysName_ = u.sysname;
  nodeName_ = u.nodename;
  release_ = u.release;
  version_ = u.version;
  machine_ = u.machine;
  domainName_ = u.domainname;

  /* GNU fuction
   * getpagesize(); numa_pagesize()
   * get_nprocs_conf();
   * get_nprocs();
   */
  pageSize_ = sysconf(_SC_PAGESIZE);
  pageNumAll_ = sysconf(_SC_PHYS_PAGES);
  pageNumAva_ = sysconf(_SC_AVPHYS_PAGES);
  memSize_ = pageSize_ * pageNumAll_ / MB;

/* Gather number of processors and CPU ticks linux2.6 Later
 refer:http://blog.csdn.net/u012317833/article/details/39476831 */
#ifdef WIN32
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  cpuConf_ = si.dwNumberOfProcessors;
#else
  cpuConf_ = sysconf(_SC_NPROCESSORS_CONF);
#endif

#ifdef WIN32
  SYSTEM_INFO TRACE;
  GetSystemInfo(&TRACE);
  cpuOnline_ = TRACE.dwNumberOfProcessors;
#else
  cpuOnline_ = sysconf(_SC_NPROCESSORS_ONLN);
#endif
  maxFileFds_ = sysconf(_SC_OPEN_MAX);
  tickSpersec_ = sysconf(_SC_CLK_TCK);
  maxHostName_ = sysconf(_SC_HOST_NAME_MAX);
  maxLoginName_ = sysconf(_SC_LOGIN_NAME_MAX);

  if (getrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
    LOG(ERROR) << "Getrlimit failed, errno:" << errno;
    return false;
  }

  maxSocket_ = (int)rlmt.rlim_cur;
  return true;
}

bool OsInfo::getMemUsage() {
  std::string pidStatusPath = "/proc/" + std::to_string(getpid()) + "/status";
  std::ifstream fin(pidStatusPath);
  if (!fin) {
    LOG(ERROR) << "Error opening " << pidStatusPath << " for input";
    return false;
  }

  std::string line;
  while (getline(fin, line)) {
    LOG(TRACE) << "Read from file: " << line;
    // kb
    if (line.find("VmSize")) {
    } else if (line.find("VmRSS")) {
    }
  }
  return true;
}

bool OsInfo::getHddUsage() {
  FILE* fp = NULL;
  char a[80], d[80], e[80], f[80], buf[256];
  double c, b;
  double dev_total = 0, dev_used = 0;

  fp = popen("df", "r");
  if (!fp) {
    return false;
  }

  if (!fgets(buf, sizeof(buf), fp)) {
    return false;
  }
  while (6 == fscanf(fp, "%s %lf %lf %s %s %s", a, &b, &c, d, e, f)) {
    dev_total += b;
    dev_used += c;
  }

  struct HddInfo hdd;
  hdd.total = dev_total / MB;
  hdd.used_rate = dev_used / dev_total * 100;
  pclose(fp);
  return true;
}

/*
  * $ cat /proc/meminfo
  * MemTotal:        8120568 kB
  * MemFree:         2298932 kB
  * Cached:          1907240 kB
  * SwapCached:            0 kB
  * SwapTotal:      15859708 kB
  * SwapFree:       15859708 kB
  */
bool OsInfo::getMemInfo(struct MemInfo& mem) {
  // https://www.cnblogs.com/JCSU/articles/1190685.html
  std::ifstream fin("/proc/meminfo");
  if (!fin) {
    LOG(ERROR) << "Cannot read /proc/meminfo, maybe /proc is not mounted yet";
    return false;
  }

  std::string line;
  while (getline(fin, line)) {
    LOG(TRACE) << "Read from file: " << line;
    if (line.find("MemTotal")) {
    } else if (line.find("MemFree")) {
    } else if (line.find("MemAvailable")) {
    } else if (line.find("Shmem")) {
    } else if (line.find("Slab")) {
    } else if (line.find("HugePages_Total")) {
    } else if (line.find("HugePages_Free")) {
    } else if (line.find("HugePages_Rsvd")) {
    } else if (line.find("HugePages_Surp")) {
    } else if (line.find("Hugepagesize")) {
    } else if (line.find("DirectMap4k")) {
    } else if (line.find("DirectMap2M")) {
    } else if (line.find("DirectMap1G")) {
    }
  }
  return true;
}

void OsInfo::cpuId(uint32_t i, uint32_t* buf) {
#if ((__i386__ || __amd64__) && (__GNUC__ || __INTEL_COMPILER))
#if (__i386__)
  /*
   * we could not use %ebx as output parameter if gcc builds PIC,
   * and we could not save %ebx on stack, because %esp is used,
   * when the -fomit-frame-pointer optimization is specified.
   */
  __asm__(
      "    mov    %%ebx, %%esi;  "
      "    cpuid;                "
      "    mov    %%eax, (%1);   "
      "    mov    %%ebx, 4(%1);  "
      "    mov    %%edx, 8(%1);  "
      "    mov    %%ecx, 12(%1); "
      "    mov    %%esi, %%ebx;  "
      :
      : "a"(i), "D"(buf)
      : "ecx", "edx", "esi", "memory");
#else /* __amd64__ */
  uint32_t eax, ebx, ecx, edx;

  __asm__("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(i));

  buf[0] = eax;
  buf[1] = ebx;
  buf[2] = edx;
  buf[3] = ecx;
#endif
#endif
}

/* auto detect the L2 cache line size of modern and widespread CPUs
 * 这个函数便是在获取CPU的信息,根据CPU的型号对ngx_cacheline_size进行设置*/
void OsInfo::cpuInit() {
#if ((__i386__ || __amd64__) && (__GNUC__ || __INTEL_COMPILER))
  char* vendor;
  uint32_t vbuf[5], cpu[4], model;

  memset(vbuf, 0, sizeof(vbuf));

  cpuId(0, vbuf);

  vendor = (char*)&vbuf[1];

  if (vbuf[0] == 0) {
    LOG(ERROR) << "Get cpu TRACE failed.";
    return;
  }

  cpuId(1, cpu);

  if (strcmp(vendor, "GenuineIntel") == 0) {
    switch ((cpu[0] & 0xf00) >> 8) {
      /* Pentium */
      case 5:
        LOG(TRACE) << "This cpu is Pentium.";
        cacheLineSize_ = 32;
        break;
      /* Pentium Pro, II, III */
      case 6:
        LOG(TRACE) << "This cpu is Pentium Pro.";
        cacheLineSize_ = 32;

        model = ((cpu[0] & 0xf0000) >> 8) | (cpu[0] & 0xf0);
        if (model >= 0xd0) {
          /* Intel Core, Core 2, Atom */
          cacheLineSize_ = 64;
        }
        break;
      /*
       * Pentium 4, although its cache line size is 64 bytes,
       * it prefetches up to two cache lines during memory read
       */
      case 15:
        LOG(TRACE) << "This cpu is Pentium 4.";
        cacheLineSize_ = 128;
        break;
      default:
        LOG(TRACE) << "This cpu is Default.";
        cacheLineSize_ = 32;
        break;
    }

  } else if (strcmp(vendor, "AuthenticAMD") == 0) {
    LOG(TRACE) << "This cpu is AMD.";
    cacheLineSize_ = 64;
  }
#else
  cacheLineSize_ = 32;
#endif
}

int64_t OsInfo::GetCurCpuTime() {
  char file_name[64] = {0};
  snprintf(file_name, sizeof(file_name), "/proc/%d/stat", getpid());

  FILE* pid_stat = fopen(file_name, "r");
  if (!pid_stat) {
    return -1;
  }

  PidStat result;
  int ret = fscanf(
      pid_stat,
      "%ld %s %c %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld",
      &result.pid, result.comm, &result.state, &result.ppid, &result.pgrp,
      &result.session, &result.tty_nr, &result.tpgid, &result.flags,
      &result.minflt, &result.cminflt, &result.majflt, &result.cmajflt,
      &result.utime, &result.stime, &result.cutime, &result.cstime);

  fclose(pid_stat);

  if (ret <= 0) {
    return -1;
  }

  return result.utime + result.stime + result.cutime + result.cstime;
}

int64_t OsInfo::GetTotalCpuTime() {
  char file_name[] = "/proc/stat";
  FILE* stat = fopen(file_name, "r");
  if (!stat) {
    return -1;
  }

  CpuStat result;
  int ret = fscanf(stat, "%s %ld %ld %ld %ld %ld %ld %ld", result.cpu_label,
                   &result.user, &result.nice, &result.system, &result.idle,
                   &result.iowait, &result.irq, &result.softirq);

  fclose(stat);

  if (ret <= 0) {
    return -1;
  }

  return result.user + result.nice + result.system + result.idle +
         result.iowait + result.irq + result.softirq;
}

float OsInfo::CalculateCurCpuUseage(int64_t cur_cpu_time_start,
                                    int64_t cur_cpu_time_stop,
                                    int64_t total_cpu_time_start,
                                    int64_t total_cpu_time_stop) {
  int64_t cpu_result = total_cpu_time_stop - total_cpu_time_start;
  if (cpu_result <= 0) {
    return 0;
  }
  return (cpuOnline_ * 100.0f * (cur_cpu_time_stop - cur_cpu_time_start)) /
         cpu_result;
}

bool OsInfo::Is32BitOS() { return !Is64BitOS(); }

bool OsInfo::Is64BitOS() {
#if defined(__APPLE__)
  return true;
#elif defined(linux) || defined(__linux) || defined(__linux__)
  struct stat buffer;
  return (stat("/lib64/ld-linux-x86-64.so.2", &buffer) == 0);
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#if defined(_WIN64)
  return true;
#elif defined(_WIN32) || defined(__CYGWIN__)
  BOOL bWow64Process = FALSE;
  return IsWow64Process(GetCurrentProcess(), &bWow64Process) && bWow64Process;
#endif
#else
#error Unsupported platform
#endif
}

bool OsInfo::Is32BitProcess() { return !Is64BitProcess(); }

bool OsInfo::Is64BitProcess() {
#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
#if defined(__x86_64__) || defined(__amd64__) || defined(__aarch64__) || \
    defined(__ia64__) || defined(__ppc64__)
  return true;
#else
  return false;
#endif
#elif defined(_WIN32) || defined(_WIN64)
#if defined(_WIN64)
  return true;
#elif defined(_WIN32)
  return false;
#endif
#else
#error Unsupported platform
#endif
}

bool OsInfo::IsBigEndian() {
  char16_t test = 0x0102;
  return ((char*)&test)[0] == 0x01;
}

bool OsInfo::IsLittleEndian() {
  char16_t test = 0x0102;
  return ((char*)&test)[0] == 0x02;
}

std::string OsInfo::OSVersion() {
#if defined(__APPLE__)
  char result[1024];
  size_t size = sizeof(result);
  if (sysctlbyname("kern.osrelease", result, &size, nullptr, 0) == 0)
    return result;

  return "<apple>";
#elif defined(__CYGWIN__)
  struct utsname name;
  if (uname(&name) == 0) {
    std::string result(name.sysname);
    result.append(" ");
    result.append(name.release);
    result.append(" ");
    result.append(name.version);
    return result;
  }

  return "<cygwin>";
#elif defined(linux) || defined(__linux) || defined(__linux__)
  static std::regex pattern("DISTRIB_DESCRIPTION=\"(.*)\"");

  std::string line;
  std::ifstream stream("/etc/lsb-release");
  while (getline(stream, line)) {
    std::smatch matches;
    if (std::regex_match(line, matches, pattern)) return matches[1];
  }

  return "<linux>";
#elif defined(_WIN32) || defined(_WIN64)
  static NTSTATUS(__stdcall* RtlGetVersion)(
      OUT PRTL_OSVERSIONINFOEXW lpVersionInformation) =
      (NTSTATUS(__stdcall*)(PRTL_OSVERSIONINFOEXW))GetProcAddress(
          GetModuleHandle("ntdll.dll"), "RtlGetVersion");
  static void(__stdcall* GetNativeSystemInfo)(OUT LPSYSTEM_INFO lpSystemInfo) =
      (void(__stdcall*)(LPSYSTEM_INFO))GetProcAddress(
          GetModuleHandle("kernel32.dll"), "GetNativeSystemInfo");
  static BOOL(__stdcall * GetProductInfo)(
      IN DWORD dwOSMajorVersion, IN DWORD dwOSMinorVersion,
      IN DWORD dwSpMajorVersion, IN DWORD dwSpMinorVersion,
      OUT PDWORD pdwReturnedProductType) =
      (BOOL(__stdcall*)(DWORD, DWORD, DWORD, DWORD, PDWORD))GetProcAddress(
          GetModuleHandle("kernel32.dll"), "GetProductInfo");

  OSVERSIONINFOEXW osvi;
  ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXW));
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

  if (RtlGetVersion != nullptr) {
    NTSTATUS ntRtlGetVersionStatus = RtlGetVersion(&osvi);
    if (ntRtlGetVersionStatus != STATUS_SUCCESS) return "<windows>";
  } else {
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)  // C4996: 'function': was declared deprecated
#endif
    BOOL bOsVersionInfoEx = GetVersionExW((OSVERSIONINFOW*)&osvi);
    if (bOsVersionInfoEx == 0) return "<windows>";
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
  }

  SYSTEM_INFO si;
  ZeroMemory(&si, sizeof(SYSTEM_INFO));

  if (GetNativeSystemInfo != nullptr)
    GetNativeSystemInfo(&si);
  else
    GetSystemInfo(&si);

  if ((osvi.dwPlatformId != VER_PLATFORM_WIN32_NT) ||
      (osvi.dwMajorVersion <= 4)) {
    return "<windows>";
  }

  std::stringstream os;

  // Windows version
  os << "Microsoft";
  if (osvi.dwMajorVersion >= 6) {
    if (osvi.dwMajorVersion == 10) {
      if (osvi.dwMinorVersion == 0) {
        if (osvi.wProductType == VER_NT_WORKSTATION)
          os << " Windows 10";
        else
          os << " Windows Server 2016";
      }
    } else if (osvi.dwMajorVersion == 6) {
      if (osvi.dwMinorVersion == 3) {
        if (osvi.wProductType == VER_NT_WORKSTATION)
          os << " Windows 8.1";
        else
          os << " Windows Server 2012 R2";
      } else if (osvi.dwMinorVersion == 2) {
        if (osvi.wProductType == VER_NT_WORKSTATION)
          os << " Windows 8";
        else
          os << " Windows Server 2012";
      } else if (osvi.dwMinorVersion == 1) {
        if (osvi.wProductType == VER_NT_WORKSTATION)
          os << " Windows 7";
        else
          os << " Windows Server 2008 R2";
      } else if (osvi.dwMinorVersion == 0) {
        if (osvi.wProductType == VER_NT_WORKSTATION)
          os << " Windows Vista";
        else
          os << " Windows Server 2008";
      }
    }

    DWORD dwType;
    if ((GetProductInfo != nullptr) &&
        GetProductInfo(osvi.dwMajorVersion, osvi.dwMinorVersion, 0, 0,
                       &dwType)) {
      switch (dwType) {
        case PRODUCT_ULTIMATE:
          os << " Ultimate Edition";
          break;
        case PRODUCT_PROFESSIONAL:
          os << " Professional";
          break;
        case PRODUCT_HOME_PREMIUM:
          os << " Home Premium Edition";
          break;
        case PRODUCT_HOME_BASIC:
          os << " Home Basic Edition";
          break;
        case PRODUCT_ENTERPRISE:
          os << " Enterprise Edition";
          break;
        case PRODUCT_BUSINESS:
          os << " Business Edition";
          break;
        case PRODUCT_STARTER:
          os << " Starter Edition";
          break;
        case PRODUCT_CLUSTER_SERVER:
          os << " Cluster Server Edition";
          break;
        case PRODUCT_DATACENTER_SERVER:
          os << " Datacenter Edition";
          break;
        case PRODUCT_DATACENTER_SERVER_CORE:
          os << " Datacenter Edition (core installation)";
          break;
        case PRODUCT_ENTERPRISE_SERVER:
          os << " Enterprise Edition";
          break;
        case PRODUCT_ENTERPRISE_SERVER_CORE:
          os << " Enterprise Edition (core installation)";
          break;
        case PRODUCT_ENTERPRISE_SERVER_IA64:
          os << " Enterprise Edition for Itanium-based Systems";
          break;
        case PRODUCT_SMALLBUSINESS_SERVER:
          os << " Small Business Server";
          break;
        case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:
          os << " Small Business Server Premium Edition";
          break;
        case PRODUCT_STANDARD_SERVER:
          os << " Standard Edition";
          break;
        case PRODUCT_STANDARD_SERVER_CORE:
          os << " Standard Edition (core installation)";
          break;
        case PRODUCT_WEB_SERVER:
          os << " Web Server Edition";
          break;
      }
    }
  } else if ((osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion == 2)) {
    if (GetSystemMetrics(SM_SERVERR2))
      os << " Windows Server 2003 R2,";
    else if (osvi.wSuiteMask & VER_SUITE_STORAGE_SERVER)
      os << " Windows Storage Server 2003";
    else if (osvi.wSuiteMask & VER_SUITE_WH_SERVER)
      os << " Windows Home Server";
    else if ((osvi.wProductType == VER_NT_WORKSTATION) &&
             (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64))
      os << " Windows XP Professional x64 Edition";
    else
      os << " Windows Server 2003,";
    if (osvi.wProductType != VER_NT_WORKSTATION) {
      if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64) {
        if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
          os << " Datacenter Edition for Itanium-based Systems";
        else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
          os << " Enterprise Edition for Itanium-based Systems";
      } else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
        if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
          os << " Datacenter x64 Edition";
        else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
          os << " Enterprise x64 Edition";
        else
          os << " Standard x64 Edition";
      } else {
        if (osvi.wSuiteMask & VER_SUITE_COMPUTE_SERVER)
          os << " Compute Cluster Edition";
        else if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
          os << " Datacenter Edition";
        else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
          os << " Enterprise Edition";
        else if (osvi.wSuiteMask & VER_SUITE_BLADE)
          os << " Web Edition";
        else
          os << " Standard Edition";
      }
    }
  } else if ((osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion == 1)) {
    os << " Windows XP";
    if (osvi.wSuiteMask & VER_SUITE_PERSONAL)
      os << " Home Edition";
    else
      os << " Professional";
  } else if ((osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion == 0)) {
    os << " Windows 2000";
    if (osvi.wProductType == VER_NT_WORKSTATION)
      os << " Professional";
    else {
      if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
        os << " Datacenter Server";
      else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
        os << " Advanced Server";
      else
        os << " Server";
    }
  }

  // Windows Service Pack version
  if (std::wcslen(osvi.szCSDVersion) > 0) os << " " << osvi.szCSDVersion;

  // Windows build
  os << " (build " << osvi.dwBuildNumber << ")";

  // Windows architecture
  if (osvi.dwMajorVersion >= 6) {
    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
      os << ", 64-bit";
    else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
      os << ", 32-bit";
  }

  return os.str();
#else
#error Unsupported platform
#endif
}

std::string OsInfo::EndLine() {
#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
  return "\n";
#elif defined(_WIN32) || defined(_WIN64)
  return "\r\n";
#else
#error Unsupported platform
#endif
}

std::string OsInfo::UnixEndLine() { return "\n"; }

std::string OsInfo::WindowsEndLine() { return "\r\n"; }

std::string OsInfo::GetEnvar(const std::string name) {
#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
  char* envar = ::getenv(name.c_str());
  return (envar != nullptr) ? std::string(envar) : std::string();
#elif defined(_WIN32) || defined(_WIN64)
  std::wstring wname = Encoding::FromUTF8(name);
  std::vector<wchar_t> buffer(MAX_PATH);

  DWORD size = ::GetEnvironmentVariableW(wname.c_str(), buffer.data(),
                                         (DWORD)buffer.size());
  if (size > buffer.size()) {
    buffer.resize(size);
    size = ::GetEnvironmentVariableW(wname.c_str(), buffer.data(),
                                     (DWORD)buffer.size());
  }

  return (size > 0) ? Encoding::ToUTF8(std::wstring(buffer.data(), size))
                    : std::string();
#endif
}

void OsInfo::SetEnvar(const std::string name, const std::string value) {
#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
  if (::setenv(name.c_str(), value.c_str(), 1) != 0) return;
#elif defined(_WIN32) || defined(_WIN64)
  if (!SetEnvironmentVariableW(Encoding::FromUTF8(name).c_str(),
                               Encoding::FromUTF8(value).c_str()))
    return
#endif
}

void OsInfo::ClearEnvar(const std::string name) {
#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
  if (::unsetenv(name.c_str()) != 0) return;
#elif defined(_WIN32) || defined(_WIN64)
  if (!SetEnvironmentVariableW(Encoding::FromUTF8(name).c_str(), nullptr))
    return;
#endif
}

void OsInfo::dbgOsInfo() {
  LOG(TRACE) << "OS type:" << sysName_;
  LOG(TRACE) << "OS nodename:" << nodeName_;
  LOG(TRACE) << "OS release:" << release_;
  LOG(TRACE) << "OS version:" << version_;
  LOG(TRACE) << "OS machine:" << machine_;
  LOG(TRACE) << "OS domainname:" << domainName_;
  LOG(TRACE) << "Processors conf:" << cpuConf_;
  LOG(TRACE) << "Processors avai:" << cpuOnline_;
  LOG(TRACE) << "Cacheline size: " << cacheLineSize_;
  LOG(TRACE) << "Pagesize: " << pageSize_;
  LOG(TRACE) << "Pages all num:" << pageNumAll_;
  LOG(TRACE) << "Pages available: " << pageNumAva_;
  LOG(TRACE) << "Memory size:  " << memSize_ << "MB";
  LOG(TRACE) << "Files max opened: " << maxFileFds_;
  LOG(TRACE) << "Socket max opened: " << maxSocket_;
  LOG(TRACE) << "Ticks per second: " << tickSpersec_;
  LOG(TRACE) << "Maxlen host name: " << maxHostName_;
  LOG(TRACE) << "Maxlen login name: " << maxLoginName_;

  LOG(TRACE) << "Memory name1: " << mem_.name1;
  LOG(TRACE) << "Memory total: " << mem_.total;
  LOG(TRACE) << "Memory name2: " << mem_.name2;
  LOG(TRACE) << "Memory free: " << mem_.free;
  LOG(TRACE) << "Memory used rate: " << mem_.used_rate;

  LOG(TRACE) << "Hdd total: " << hdd_.total;
  LOG(TRACE) << "Hdd used_rate: " << hdd_.used_rate;
}

bool OsInfo::osInit() {
  sysInit();
  vnodeInit();
  dbgOsInfo();
  return true;
}

}  // namespace MSF
