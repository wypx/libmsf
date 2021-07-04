
#include <base/logging.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <malloc.h>
#include <paths.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>
#include <termios.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <iosfwd>
#include <fstream>
#include <base/logging.h>

/// the structure corresponding the content in /proc/<pid>/stat
struct ProcStat {
  int pid;                        // The process id.
  char command[_POSIX_PATH_MAX];  // The filename of the executable
  char task_state;                // 1        // R is running, S is sleeping,
                                  // D is sleeping in an uninterruptible wait,
                                  // Z is zombie, T is traced or stopped
  int ppid;                       // The pid of the parent.
  int pgid;                       // The pgid of the process.
  int sid;                        // The sid id of the process.
  int tty_nr;                     // The tty_nr the process uses
  int tty_pgrp;                   // (too long)
  uint32_t task_flags;            // The task_flags of the process.
  uint32_t min_flt;               // The number of minor faults
  uint32_t cmin_flt;              // The number of minor faults with childs
  uint32_t maj_flt;               // The number of major faults
  uint32_t cmaj_flt;              // The number of major faults with childs
  int utime;                      // user mode jiffies
  int stime;                      // kernel mode jiffies
  int cutime;                     // user mode jiffies with childs
  int cstime;                     // kernel mode jiffies with childs
  int priority;                   // process's next timeslice
  int nice;                       // the standard nice value, plus fifteen
  uint32_t num_threads;           // The time in jiffies of the next num_threads
  uint32_t it_real_value;  // The time before the next SIGALRM is sent to the
                           // process
  int start_time;  // 20        // Time the process started after system boot
  uint64_t vsize;  // Virtual memory size
  uint64_t rss;    // Resident Set Size
  uint64_t rlim;   // Current limit in bytes on the rss
  uint64_t start_code;   // The address above which program text can run
  uint64_t end_code;     // The address below which program text can run
  uint64_t start_stack;  // The address of the start of the stack
  uint64_t kstkesp;      // The current value of ESP
  uint64_t kstkeip;      // The current value of EIP
  int pendingsig;        // The bitmap of pending pendingsigs
  int block_sig;         // 30         // The bitmap of block_sig pendingsigs
  int sigign;            // The bitmap of ignored pendingsigs
  int sigcatch;          // The bitmap of catched pendingsigs
  uint64_t wchan;        // 33       // (too long)
  uint64_t nswap;        // The num of swapped pages
  int cnswap;            // The num of swapped pages in all subprocess
  int exit_signal;  // The signal to parent process when current process ends
  int task_cpu;     // Which cpu current task run in
  int task_rt_priority;  // Relative priority of real-time processes
  int task_policy;       // The scheduling strategy of process
                         // 0 is Non real-time process
                         // 1 is FIFO real-time process
                         // 2 is RR real-time process

  uint32_t euid;  // effective user id
  uint32_t egid;  // effective group id

 public:
  ProcStat() { update(); }
  ProcStat(pid_t pid) { update(pid); }

  /// Get statistics of the process according to pid
  bool update(pid_t pid) {
    std::string statfile("/proc/" + std::to_string((unsigned)pid) + "/stat");

    if (-1 == access(statfile.c_str(), R_OK)) {
      return false;
    }

    struct stat st;
    if (-1 != stat(statfile.c_str(), &st)) {
      euid = st.st_uid;
      egid = st.st_gid;
    } else {
      euid = egid = (unsigned int)-1;
    }

    std::ifstream in(statfile);
    if (!in.is_open()) {
      return false;
    }

    std::string temp;
    in >> this->pid;
    in >> temp;
    memcpy(command, temp.substr(1, temp.length() - 2).c_str(),
           temp.length() - 1);

    in >> task_state >> ppid >> pgid >> sid >> tty_nr >> tty_pgrp >>
        task_flags >> min_flt >> cmin_flt >> maj_flt >> cmaj_flt >> utime >>
        stime >> cutime >> cstime >> priority >> nice >> num_threads >>
        it_real_value >> start_time >> vsize >> rss >> rlim >> start_code >>
        end_code >> start_stack >> kstkesp >> kstkeip >> pendingsig >>
        block_sig >> sigign >> sigcatch >> wchan >> nswap >> cnswap >>
        exit_signal >> task_cpu >> task_rt_priority >> task_policy;

    return true;
  }

  /// Get statistics of the process where this function is called
  bool update() { return update(getpid()); }

  friend std::ostream &operator<<(std::ostream &out, ProcStat const &stat) {

#define set_value(value)                                                   \
  out << "| " << std::left << std::setw(16) << #value << "|" << std::right \
      << std::setw(24) << stat.value << " |" << std::endl

    std::string division(45, '-');
    out << division << std::endl;
    set_value(pid);
    set_value(command);
    set_value(task_state);
    set_value(euid);
    set_value(egid);
    set_value(ppid);
    set_value(pgid);
    set_value(sid);
    set_value(tty_nr);
    set_value(tty_pgrp);
    set_value(task_flags);
    set_value(min_flt);
    set_value(cmin_flt);
    set_value(maj_flt);
    set_value(cmaj_flt);
    set_value(utime);
    set_value(stime);
    set_value(cutime);
    set_value(cstime);
    set_value(priority);
    set_value(nice);
    set_value(num_threads);
    set_value(it_real_value);
    set_value(start_time);
    set_value(vsize);
    set_value(rss);
    set_value(rlim);
    set_value(start_code);
    set_value(end_code);
    set_value(start_stack);
    set_value(kstkesp);
    set_value(kstkeip);
    set_value(pendingsig);
    set_value(block_sig);
    set_value(sigign);
    set_value(sigcatch);
    set_value(wchan);
    set_value(nswap);
    set_value(cnswap);
    set_value(exit_signal);
    set_value(task_cpu);
    set_value(task_rt_priority);
    set_value(task_policy);
    out << division << std::endl;

    return out;
  }
};

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

std::string PidName(pid_t pid) {
  std::string name = "/proc/" + std::to_string(pid) + "/stat";
  FILE *pid_stat = ::fopen(name.c_str(), "r");
  if (!pid_stat) {
    return "";
  }

  PidStat result;
  int ret = ::fscanf(
      pid_stat,
      "%ld %s %c %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld",
      &result.pid, result.comm, &result.state, &result.ppid, &result.pgrp,
      &result.session, &result.tty_nr, &result.tpgid, &result.flags,
      &result.minflt, &result.cminflt, &result.majflt, &result.cmajflt,
      &result.utime, &result.stime, &result.cutime, &result.cstime);

  ::fclose(pid_stat);

  if (ret <= 0) {
    return "";
  }
  name = result.comm;
  name.erase(0);
  name.erase(name.end());
  return name;
}

// https://github.com/PeaJune/pidof/blob/master/pidof.cpp
bool PidOF(const std::string &name, std::vector<pid_t> &vec) {
  DIR *dir = ::opendir("/proc");
  if (!dir) {
    LOG(ERROR) << "failed to open proc directory";
    return false;
  }
  pid_t pid;
  struct dirent *de = nullptr;
  while ((de = ::readdir(dir)) != nullptr) {
    // char *e;
    // pid_t pid = ::strtol(de->d_name, &e, 10);
    // if (*e != 0) continue;
    if ((pid = ::atoi(de->d_name)) == 0) continue;
    if (name == PidName(pid)) {
      vec.push_back(pid);
    }
  }
  ::closedir(dir);
  return true;
}

bool KillPid(pid_t pid) {
  int deadcnt = 20;
  struct stat st;

  std::string name = "/proc/" + std::to_string(pid) + "/stat";
  ::kill(pid, SIGTERM);
  while (deadcnt--) {
    ::usleep(100 * 1000);
    if ((::stat(name.c_str(), &st) > -1) && (st.st_mode & S_IFREG)) {
      ::kill(pid, SIGKILL);
    } else {
      break;
    }
  }
  return deadcnt > 0;
}

// https://www.cnblogs.com/leeming0222/articles/3994125.html
bool KillAll(const std::string &name) {
  std::vector<pid_t> pids;
  if (!PidOF(name, pids)) {
    return false;
  }
  for (auto pid : pids) {
    if (!KillPid(pid)) {
      return false;
    }
  }
  pids.clear();
  if (!PidOF(name, pids)) {
    return false;
  }
  return pids.empty();
}