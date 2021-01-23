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
#ifndef BASE_OS_H_
#define BASE_OS_H_

#include <grp.h>
#include <numa.h>
#include <pwd.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>

#include "plugin.h"

namespace MSF {

#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || \
    defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || \
    defined(__NT__) || defined(WINCE) || defined(_WIN32_WCE)
#define OS_WINDOWS 1
#define PLATFORM_ID "Windows"

#elif defined(linux) || defined(__linux__) || defined(__linux)
#define OS_LINUX 1
#define PLATFORM_ID "Linux"

#elif defined(__CYGWIN__)
#define OS_CYGWIN 1
#define PLATFORM_ID "Cygwin"

#elif defined(__MINGW32__)
#define OS_MINGW32 1
#define PLATFORM_ID "MinGW"

#elif defined(__APPLE__)
#define OS_APPLE 1
#define PLATFORM_ID "Darwin"

#elif defined(__FreeBSD__) || defined(__FreeBSD)
#define PLATFORM_ID "FreeBSD"

#elif defined(__NetBSD__) || defined(__NetBSD)
#define PLATFORM_ID "NetBSD"

#elif defined(__OpenBSD__) || defined(__OPENBSD)
#define PLATFORM_ID "OpenBSD"

#elif defined(__sun) || defined(sun)
#define PLATFORM_ID "SunOS"

#elif defined(_AIX) || defined(__AIX) || defined(__AIX__) || defined(__aix) || \
    defined(__aix__)
#define PLATFORM_ID "AIX"

#elif defined(__sgi) || defined(__sgi__) || defined(_SGI)
#define PLATFORM_ID "IRIX"

#elif defined(__hpux) || defined(__hpux__)
#define PLATFORM_ID "HP-UX"

#elif defined(__HAIKU__)
#define PLATFORM_ID "Haiku"

#elif defined(__BeOS) || defined(__BEOS__) || defined(_BEOS)
#define PLATFORM_ID "BeOS"

#elif defined(__QNX__) || defined(__QNXNTO__)
#define PLATFORM_ID "QNX"

#elif defined(__tru64) || defined(_tru64) || defined(__TRU64__)
#define PLATFORM_ID "Tru64"

#elif defined(__riscos) || defined(__riscos__)
#define PLATFORM_ID "RISCos"

#elif defined(__sinix) || defined(__sinix__) || defined(__SINIX__)
#define PLATFORM_ID "SINIX"

#elif defined(__UNIX_SV__)
#define PLATFORM_ID "UNIX_SV"

#elif defined(__bsdos__)
#define PLATFORM_ID "BSDOS"

#elif defined(_MPRAS) || defined(MPRAS)
#define PLATFORM_ID "MP-RAS"

#elif defined(__osf) || defined(__osf__)
#define PLATFORM_ID "OSF1"

#elif defined(_SCO_SV) || defined(SCO_SV) || defined(sco_sv)
#define PLATFORM_ID "SCO_SV"

#elif defined(__ultrix) || defined(__ultrix__) || defined(_ULTRIX)
#define PLATFORM_ID "ULTRIX"

#elif defined(__XENIX__) || defined(_XENIX) || defined(XENIX)
#define PLATFORM_ID "Xenix"

#elif defined(__WATCOMC__)
#if defined(__LINUX__)
#define PLATFORM_ID "Linux"

#elif defined(__DOS__)
#define PLATFORM_ID "DOS"

#elif defined(__OS2__)
#define PLATFORM_ID "OS2"

#elif defined(__WINDOWS__)
#define PLATFORM_ID "Windows3x"

#else /* unknown platform */
#define PLATFORM_ID
#endif

#else /* unknown platform */
#define PLATFORM_ID

#endif

/* For windows compilers MSVC and Intel we can determine
   the architecture of the compiler being used.  This is because
   the compilers do not have flags that can change the architecture,
   but rather depend on which compiler is being used
*/
#if defined(_WIN32) && defined(_MSC_VER)
#if defined(_M_IA64)
#define ARCHITECTURE_ID "IA64"

#elif defined(_M_X64) || defined(_M_AMD64)
#define ARCHITECTURE_ID "x64"

#elif defined(_M_IX86)
#define ARCHITECTURE_ID "X86"

#elif defined(_M_ARM64)
#define ARCHITECTURE_ID "ARM64"

#elif defined(_M_ARM)
#if _M_ARM == 4
#define ARCHITECTURE_ID "ARMV4I"
#elif _M_ARM == 5
#define ARCHITECTURE_ID "ARMV5I"
#else
#define ARCHITECTURE_ID "ARMV" STRINGIFY(_M_ARM)
#endif

#elif defined(_M_MIPS)
#define ARCHITECTURE_ID "MIPS"

#elif defined(_M_SH)
#define ARCHITECTURE_ID "SHx"

#else /* unknown architecture */
#define ARCHITECTURE_ID ""
#endif

#elif defined(__WATCOMC__)
#if defined(_M_I86)
#define ARCHITECTURE_ID "I86"

#elif defined(_M_IX86)
#define ARCHITECTURE_ID "X86"

#else /* unknown architecture */
#define ARCHITECTURE_ID ""
#endif

#elif defined(__IAR_SYSTEMS_ICC__) || defined(__IAR_SYSTEMS_ICC)
#if defined(__ICCARM__)
#define ARCHITECTURE_ID "ARM"

#elif defined(__ICCAVR__)
#define ARCHITECTURE_ID "AVR"

#else /* unknown architecture */
#define ARCHITECTURE_ID ""
#endif
#else
#define ARCHITECTURE_ID
#endif

#define KB (1024)
#define MB (1024 * KB)
#define GB (1024 * MB)
#define TB (1024 * GB)

struct MemInfo {
  char name1[20];
  uint64_t total;
  char name2[20];
  uint64_t free;
  uint32_t used_rate;
};

/** Memory information structure */
struct msf_mem {
  uint32_t total_size;  /** total size of memory pool */
  uint32_t free;        /** free memory */
  uint32_t used;        /** allocated size */
  uint32_t real_used;   /** used size plus overhead from malloc */
  uint32_t max_used;    /** maximum used size since server start? */
  uint32_t min_frag;    /** minimum number of fragmentations? */
  uint32_t total_frags; /** number of total memory fragments */
};

struct HddInfo {
  uint64_t total;
  uint32_t used_rate;
};

/* CPU信息描述
 * (user, nice, system,idle, iowait, irq, softirq, stealstolen, guest)的9元组
 *
 * 系统命令 : top命令查看任务管理器实时系统信息
 * 1. CPU使用率: cat /proc/stat
 *    用户模式(user)
 *    低优先级的用户模式(nice)
 *    内核模式(system)
 *    空闲的处理器时间(idle)
 *    CPU利用率 = 100*(user+nice+system)/(user+nice+system+idle)
 *    第一行: cpu 711 56 2092 7010 104 0 20 0 0 0
 *    上面共有10个值 (单位:jiffies)前面8个值分别为:
 *
 *    User time, 711(用户时间)             Nice time, 56 (Nice时间)
 *    System time, 2092(系统时间)          Idle time, 7010(空闲时间)
 *    Waiting time, 104(等待时间)          Hard Irq time, 0(硬中断处理时间)
 *    SoftIRQ time, 20(软中断处理时间)     Steal time, 0(丢失时间)
 *
 *    CPU时间=user+system+nice+idle+iowait+irq+softirq+Stl
 *
 *
 * 2. 内存使用 : cat /proc/meminfo
 *    当前内存的使用量(cmem)以及内存总量(amem)
 *    内存使用百分比 = 100 * (cmem / umem)
 * 3. 网络负载 : cat /proc/net/dev
 *    从本机输出的数据包数,流入本机的数据包数
 *    平均网络负载 = (输出的数据包+流入的数据包)/2
 */

// /proc/pid/stat字段定义
struct PidStat {
  int64_t pid;
  char comm[256];
  char state;
  int64_t ppid;
  int64_t pgrp;
  int64_t session;
  int64_t tty_nr;
  int64_t tpgid;
  int64_t flags;
  int64_t minflt;
  int64_t cminflt;
  int64_t majflt;
  int64_t cmajflt;
  int64_t utime;
  int64_t stime;
  int64_t cutime;
  int64_t cstime;
  // ...
};

// /proc/stat/cpu信息字段定义
struct CpuStat {
  char cpu_label[16];
  int64_t user;
  int64_t nice;
  int64_t system;
  int64_t idle;
  int64_t iowait;

  /* percent values */
  int64_t irq;         /* Overall CPU usage */
  int64_t softirq;     /* user space (user + nice) */
  int64_t stealstolen; /* kernel space percent     */
  int64_t guest;
  int64_t avg_all;
  int64_t avg_user;
  int64_t avg_kernel;
};

struct CpuInfo {};

class OsInfo {
 public:
  OsInfo();
  ~OsInfo();
  bool getMemUsage();
  bool getHddUsage();
  bool getMemInfo(struct MemInfo &mem);
  bool getHddInfo(struct HddInfo &hdd);
  bool getCpuInfo(struct CpuInfo &cpu);
  void cpuId(uint32_t i, uint32_t *buf);
  void cpuInit();
  int64_t GetCurCpuTime();
  int64_t GetTotalCpuTime();
  float CalculateCurCpuUseage(int64_t cur_cpu_time_start,
                              int64_t cur_cpu_time_stop,
                              int64_t total_cpu_time_start,
                              int64_t total_cpu_time_stop);

  uint32_t getSuggestThreadNum();

  bool setUser(uid_t user, const std::string &userName);
  bool disablePageOut();
  bool oomAdjust();
  uint32_t getMaxOpenFds();
  bool setMaxOpenFds(const uint64_t maxOpenFds);
  static bool EnableCoredump();
  bool setCoreDumpSize(const uint64_t maxCoreSize = 2 * MB);
  bool setCoreUsePid();
  bool setCorePath(const std::string &path = "/home/core/");

  void dumpBuffer(const char *buff, size_t count, FILE *fp);
  void writeStackTrace(const std::string &filePath);
  const std::string stackTrace(bool demangle);

  static OsInfo &getInstance() {
    static OsInfo intance;
    return intance;
  }

  void GetLoadAverage();

  bool Reboot();
  void set_hostname(const std::string &hostname) { hostname_ = hostname; }
  bool set_hostname_persist(const std::string &hostname);
  const std::string hostname() const { return hostname_; }

  bool Is32BitOS();
  bool Is64BitOS();
  bool Is32BitProcess();
  bool Is64BitProcess();
  bool IsBigEndian();
  bool IsLittleEndian();
  std::string OSVersion();
  std::string EndLine();
  std::string UnixEndLine();
  std::string WindowsEndLine();
  std::string GetEnvar(const std::string name);
  void SetEnvar(const std::string name, const std::string value);
  void ClearEnvar(const std::string name);
  std::string GetSystemName();

 private:
  std::string sysName_;
  std::string nodeName_;
  std::string release_;
  std::string version_;
  std::string machine_;
  std::string domainName_; /* _UTSNAME_DOMAIN_LENGTH */

  std::string hostname_;

  /*
   * 如果能知道CPU cache行的大小,那么就可以有针对性地设置内存的对齐值,
   * 这样可以提高程序的效率.有分配内存池的接口, Nginx会将内存池边界
   * 对齐到 CPU cache行大小32位平台, cacheline_size=32 */
  uint32_t cacheLineSize_;
  /* 返回一个分页的大小,单位为字节(Byte).
   * 该值为系统的分页大小,不一定会和硬件分页大小相同*/
  uint32_t pageSize_;
  /* pagesize为4M, pagesize_shift应该为12
   * pagesize进行移位的次数, 见for (n = ngx_pagesize;
   * n >>= 1; ngx_pagesize_shift++) {  }
   */
  uint32_t pageShift_;
  uint32_t pageNumAll_;
  uint32_t pageNumAva_;
  uint64_t memSize_;
  uint32_t cpuConf_;
  uint32_t cpuOnline_;
  uint32_t maxFileFds_;
  uint32_t maxSocket_;
  uint32_t tickSpersec_; /* CPU ticks (Kernel setting) */
  uint32_t maxHostName_;
  uint32_t maxLoginName_;

  struct CpuInfo cpu_;
  struct MemInfo mem_;
  struct msf_mem meminfo;
  struct HddInfo hdd_;

  bool enableNuma_;
  uint32_t numaCount_;

  bool sysInit();
  void vnodeInit(void);
  void dbgOsInfo();
  bool osInit();
};

}  // namespace MSF
#endif