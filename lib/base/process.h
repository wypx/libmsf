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
#ifndef BASE_PROCESS_H_
#define BASE_PROCESS_H_

// #include <errno.h> /* program_invocation_short_name */
// #include <signal.h>
// #include <stdio.h>
// #include <sys/file.h>
// #include <sys/stat.h>
// #include <sys/syscall.h>
// #include <sys/types.h>
// #include <sys/wait.h>
// #include <unistd.h>

// #include <algorithm>
// #include <atomic>  // std::atomic_flag
// #include <iostream>
// #include <thread>  // std::thread
// #include <sys/prctl.h>
// #include <sys/socket.h>

// #include "gcc_attr.h"
// #include "plugin.h"

#include <base/flock.h>
#include <base/time_stamp.h>

#include <map>
#include <vector>

using namespace MSF;

namespace MSF {

/* Supper App Service Process: Process Manager*/
/* Micro service framework manager*/
struct msf_daemon {
  int msf_state;
  char msf_author[32];
  char msf_version[32];
  char msf_desc[64];
  char *msf_conf;
  char *msf_name;
  uint32_t msf_proc_num;
  struct process_desc *msf_proc_desc;
};

#define MSF_INVALID_PID -1
#define MSF_MAX_PROCESSES 64

enum msf_process_type {
  MSF_PROCESS_MASTER = 0,
  MSF_PROCESS_ROUTER,
  MSF_PROCESS_WORKER,
  MSF_PROCESS_MAXIDX,
};

struct exec_conf {
  char *path;
  char *name;
  char *const *argv;
  char *const *envp;
};

enum process_state {
  proc_state_init,
  proc_state_start,
  proc_state_stop,
};

extern char *program_invocation_name;
// extern char *program_invocation_short_name;

struct CpuTime {
  double userSeconds;
  double systemSeconds;

  CpuTime() : userSeconds(0.0), systemSeconds(0.0) {}

  double total() const { return userSeconds + systemSeconds; }
};
CpuTime cpuTime();

/////////
enum process_state state;
// pid_t  pid;
uint32_t role; /* master, slave */
uint32_t worker_processes;
char *working_directory;
int channel[2];

// char  *username;
uid_t user;
gid_t group;

int priority; /* nice of process or thread */
int cpu_affinity;
int cpu_affinity_id;

uint32_t coredump;
char *core_file;
char *lock_file;
//修改工作进程的打开文件数的最大值限制(RLIMIT_NOFILE),用于在不重启主进程的情况下增大该限制。
int rlimit_nofile;
//修改工作进程的core文件尺寸的最大值限制(RLIMIT_CORE),用于在不重启主进程的情况下增大该限制。
int rlimit_core;

char proc_name[32];
char proc_author[32];
char proc_desc[32];
char proc_version[32];

int logfd;

char *confile;
int proc_signo;
bool proc_daemon;
uint32_t proc_upgrade;
uint32_t terminate;
uint32_t quit; /* gracefully shutting down */
uint32_t fault_times;
uint32_t alarm_times;
uint32_t alarm_state;
uint32_t start_times;

uint32_t proc_svc_num;
struct svcinst *proc_svcs;

int process_try_lock(const char *proc_name, uint32_t mode);
int process_spwan(struct process_desc *proc_desc);

struct ProcessDesc {
  ProcessDesc() {}
  ~ProcessDesc() {}

#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
  pid_t pid_;
#elif defined(WIN32) || defined(WIN64)
  DWORD pid_;
  HANDLE process_;
#endif

  std::string name_;
  std::string path_;
  std::string conf_;
  time_t age_;
  int restart_times_ = 0;

  PosixFileLock *flock_;
};

class Process {
 public:
  Process(const Process &process);
  Process(Process &&process) noexcept;
  Process(uint64_t pid);
#if defined(WIN32) || defined(WIN64)
  Process(DWORD pid, HANDLE process);
#endif
  ~Process();

  Process &operator=(const Process &process);
  Process &operator=(Process &&process) noexcept;

  // Get the process Id
  uint64_t pid() const noexcept;

  /* Writes the pid to the specified file. If that fails 0 is
   * returned, otherwise the pid.
   */
  static bool SavePidFile(const char *pidfile);

  /* Remove the the specified file. The result from unlink(2)
   * is returned
   */
  static bool RemovePidFile(const char *pidfile);

  /* Reads the specified pidfile and returns the read pid.
   * 0 is returned if either there's no pidfile, it's empty
   * or no pid can be read.
   */
  static pid_t ReadPid(const char *pidfile);

  /* Reads the pid using read_pid and looks up the pid in the process
   * table (using /proc) to determine if the process already exists. If
   * so the pid is returned, otherwise 0.
   */
  static pid_t CheckPid(const char *pidfile);

  // Is the process is running?
  bool IsRunning() const;

  // Kill the process
  void Kill();

  /* Wait the process to exit
   * Will block.
   * return Process exit result
   */
  int Wait();

  /* Wait the process to exit for the given timespan
   * Will block for the given timespan in the worst case.
   * param timespan - Timespan to wait for the process
   * return Process exit result (std::numeric_limits<int>::min() in case of
   * timeout)
   */
  int WaitFor(const time_t &t);

  /* Wait the process to exit until the given timestamp
   * Will block until the given timestamp in the worst case.
   * param timestamp - Timestamp to stop wait for the process
   * return Process exit result (std::numeric_limits<int>::min() in case of
   * timeout)
   */
  int WaitUntil(const TimeStamp &timestamp);

  /* Get the current process Id
   * return Current process Id
   */
  static uint64_t CurrentProcessId();

  /* Get the parent process Id
   * return Parent process Id
   */
  static uint64_t ParentProcessId();

  /* Get the current process
   * return Current process
   */
  static Process CurrentProcess();

  /* Get the parent process
   * return Parent process
   */
  static Process ParentProcess();

  /* Exit the current process
   * param result - Result is returned to the parent
   */
  static void Exit(int result);

  static bool Spwan(ProcessDesc &desc);
  /* Execute a new process
   * If input/output/error communication pipe is not provided then
   * new process will use equivalent standard stream of the parent process.
   *
   * param command - Command to execute
   * param arguments - Pointer to arguments vector (default is nullptr)
   * param envars - Pointer to environment variables map (default is nullptr)
   * param directory - Initial working directory (default is nullptr)
   * param input - Input communication pipe (default is nullptr)
   * param output - Output communication pipe (default is nullptr)
   * param error - Error communication pipe (default is nullptr)
   * return Created process
   */
  static Process Execute(
      const std::string &command,
      const std::vector<std::string> *arguments = nullptr,
      const std::map<std::string, std::string> *envars = nullptr,
      const std::string *directory = nullptr, int input = 0, int output = 1,
      int error = 2);

#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
  pid_t pid_;
#elif defined(WIN32) || defined(WIN64)
  DWORD pid_;
  HANDLE process_;
#endif
  ProcessDesc desc_;
};

struct process_desc {
  pid_t proc_pid;
  time_t proc_age;
  int proc_fd;
  int restart_times;

  uint32_t proc_idx;
  char proc_name[64];
  char proc_path[64]; /* Check access first*/
  char proc_conf[64]; /* Check access first*/
};

#if 0
class Process
{
public:
    Process(const Process& process);
    Process(Process&& process) noexcept;
    ~Process();

    Process& operator=(const Process& process);
    Process& operator=(Process&& process) noexcept;

    //! Get the process Id
    uint64_t pid() const noexcept;

    //! Is the process is running?
    bool IsRunning() const;

    //! Kill the process
    void Kill();

    //! Wait the process to exit
    /*!
        Will block.

        \return Process exit result
    */
    int Wait();

    //! Wait the process to exit for the given timespan
    /*!
        Will block for the given timespan in the worst case.

        \param timespan - Timespan to wait for the process
        \return Process exit result (std::numeric_limits<int>::min() in case of timeout)
    */
    int WaitFor(const Timespan& timespan);
    //! Wait the process to exit until the given timestamp
    /*!
        Will block until the given timestamp in the worst case.

        \param timestamp - Timestamp to stop wait for the process
        \return Process exit result (std::numeric_limits<int>::min() in case of timeout)
    */
    int WaitUntil(const UtcTimestamp& timestamp)
    { return WaitFor(timestamp - UtcTimestamp()); }

    //! Get the current process Id
    /*!
        \return Current process Id
    */
    static uint64_t CurrentProcessId() noexcept;
    //! Get the parent process Id
    /*!
        \return Parent process Id
    */
    static uint64_t ParentProcessId() noexcept;

    //! Get the current process
    /*!
        \return Current process
    */
    static Process CurrentProcess();
    //! Get the parent process
    /*!
        \return Parent process
    */
    static Process ParentProcess();

    //! Exit the current process
    /*!
        \param result - Result is returned to the parent
    */
    static void Exit(int result);

    //! Execute a new process
    /*!
        If input/output/error communication pipe is not provided then
        new process will use equivalent standard stream of the parent
        process.

        \param command - Command to execute
        \param arguments - Pointer to arguments vector (default is nullptr)
        \param envars - Pointer to environment variables map (default is nullptr)
        \param directory - Initial working directory (default is nullptr)
        \param input - Input communication pipe (default is nullptr)
        \param output - Output communication pipe (default is nullptr)
        \param error - Error communication pipe (default is nullptr)
        \return Created process
    */
    static Process Execute(const std::string& command, const std::vector<std::string>* arguments = nullptr, const std::map<std::string, std::string>* envars = nullptr, const std::string* directory = nullptr, Pipe* input = nullptr, Pipe* output = nullptr, Pipe* error = nullptr);

    //! Swap two instances
    void swap(Process& process) noexcept;
    friend void swap(Process& process1, Process& process2) noexcept;

private:
    class Impl;

    Impl& impl() noexcept { return reinterpret_cast<Impl&>(_storage); }
    const Impl& impl() const noexcept { return reinterpret_cast<Impl const&>(_storage); }

    static const size_t StorageSize = 16;
#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
    static const size_t StorageAlign = 4;
#else
    static const size_t StorageAlign = 8;
#endif
    std::aligned_storage<StorageSize, StorageAlign>::type _storage;

    Process();
    Process(uint64_t pid);
};
#endif

}  // namespace MSF
#endif