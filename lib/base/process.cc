#include "process.h"

// #include <assert.h>
// #include <base/logging.h>
// #include <dirent.h>
// #include <pwd.h>
// #include <stdio.h>  // snprintf
// #include <stdlib.h>
// #include <sys/resource.h>
// #include <sys/times.h>
// #include <unistd.h>

// #include <algorithm>

#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
// #include <stdlib.h>
#elif defined(WIN32) || defined(WIN64)
#include <tlhelp32.h>
#include <windows.h>
#endif
#include <base/logging.h>
#include <base/thread.h>
#include <sock/sock_utils.h>

using namespace MSF;

namespace MSF {

uint64_t Process::pid() const noexcept { return static_cast<uint64_t>(pid_); }

bool Process::SavePidFile(const char *pidfile) {
  FILE *f;
  int fd;
  pid_t pid;

  if ((fd = ::open(pidfile, O_RDWR | O_CREAT, 0644)) == -1) {
    return false;
  }

  if ((f = ::fdopen(fd, "r+")) == nullptr) {
    ::close(fd);
    return false;
  }

#ifdef HAVE_FLOCK
  if (::flock(fd, LOCK_EX | LOCK_NB) == -1) {
    ::fclose(f);
    return false;
  }
#endif

  pid = ::getpid();
  if (!fprintf(f, "%ld\n", (long)pid)) {
    ::fclose(f);
    return false;
  }
  ::fflush(f);

#ifdef HAVE_FLOCK
  if (::flock(fd, LOCK_UN) == -1) {
    ::fclose(f);
    return false;
  }
#endif
  ::fclose(f);

  return true;
}

bool Process::RemovePidFile(const char *pidfile) {
  return ::unlink(pidfile) == 0;
}

pid_t Process::ReadPid(const char *pidfile) {
  FILE *f;
  long pid;

  if (!(f = ::fopen(pidfile, "r"))) return 0;
  if (::fscanf(f, "%20ld", &pid) != 1) pid = 0;
  ::fclose(f);
  return (pid_t)pid;
}

pid_t Process::CheckPid(const char *pidfile) {
  pid_t pid = Process::ReadPid(pidfile);

  /* Amazing ! _I_ am already holding the pid file... */
  if ((!pid) || (pid == ::getpid())) return 0;

  /*
   * The 'standard' method of doing this is to try and do a 'fake' kill
   * of the process.  If an ESRCH error is returned the process cannot
   * be found -- GW
   * But... errno is usually changed only on error
   */
  errno = 0;
  if (::kill(pid, 0) && errno == ESRCH) return 0;

  return pid;
}

bool IsProcessRunning(long pid) {
#if defined(WIN32)
  HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);
  DWORD ret = WaitForSingleObject(process, 0);
  CloseHandle(process);
  return (ret == WAIT_TIMEOUT);
#else
  return (::kill(pid, 0) == 0);
#endif
}

bool Process::IsRunning() const {
#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
  return (::kill(pid_, 0) == 0);
#elif defined(WIN32) || defined(WIN64)
  if (process_ == nullptr) {
    LOG(ERROR) << "failed to get exit code for a process with pid: " << pid();
    return false;
  }
  DWORD exit_code;
  if (!GetExitCodeProcess(process_, &exit_code)) {
    LOG(ERROR) << "failed to get exit code for a process with pid: " << pid();
    return false;
  }
  return (exit_code == STILL_ACTIVE);
#endif
}

void Process::Kill() {
#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
  int result = ::kill(pid_, SIGKILL);
  if (result != 0) {
    LOG(ERROR) << "failed to kill a process with pid: " << pid();
    return;
  }
#elif defined(WIN32) || defined(WIN64)
  if (process_ == nullptr) {
    LOG(ERROR) << "failed to kill a process with pid: " << pid();
    return;
  }
  if (!TerminateProcess(process_, 666)) {
    LOG(ERROR) << "failed to kill a process with pid: " << pid();
    return;
  }
#endif
}

int Process::Wait() {
#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
  int status;
  pid_t result;

  /* wait不能使用在while中, 因为wait是阻塞的,比如当某子进程一直不停止,
   * 它将一直阻塞, 从而影响了父进程.于是使用waitpid, options设置为WNOHANG,
   * 可以在非阻塞情况下处理多个退出子进程
   */
  do {
    result = ::waitpid(pid_, &status, 0);
  } while ((result < 0) && (errno == EINTR));

  if (result == -1) {
    LOG(ERROR) << "failed to wait a process with pid: " << pid();
    return -1;
  }
  if (WIFEXITED(status)) {
    LOG(INFO) << "process " << pid()
              << " normal exit, status: " << WEXITSTATUS(status);
    return WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    LOG(ERROR) << "process: " << pid()
               << " was killed by signal: " << WTERMSIG(status);
  } else if (WIFSTOPPED(status)) {
    LOG(ERROR) << "process: " << pid()
               << " was stop by signal: " << WSTOPSIG(status);
  } else if (WIFCONTINUED(status)) {
    LOG(ERROR) << "process: " << pid() << " was continued by signal SIGCONT";
  } else {
    LOG(ERROR) << "process: " << pid() << " has unknown wait status";
  }
  return -1;
#elif defined(WIN32) || defined(WIN64)
  if (process_ == nullptr) {
    LOG(ERROR) << "failed to wait a process with pid: " << pid();
    return -1;
  }
  DWORD result = ::WaitForSingleObject(process_, INFINITE);
  if (result != WAIT_OBJECT_0) {
    LOG(ERROR) << "failed to wait a process with pid: " << pid();
  }

  DWORD exit_code;
  if (!::GetExitCodeProcess(process_, &exit_code)) {
    LOG(ERROR) << "failed to get exit code for a process with id: " << pid();
  }
  return (int)exit_code;
#endif
}

int Process::WaitFor(const time_t &t) {
#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
  int status;
  pid_t result;

  do {
    result = ::waitpid(pid_, &status, WNOHANG);
    if (result == 0) Thread::YieldCurrentThread();
  } while ((result == 0) || ((result < 0) && (errno == EINTR)));

  if (result == -1) {
    LOG(ERROR) << "failed to wait a process with pid: " << pid();
    return -1;
  }
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    LOG(ERROR) << "process: " << pid()
               << " was killed by signal: " << WTERMSIG(status);
  } else if (WIFSTOPPED(status)) {
    LOG(ERROR) << "process: " << pid()
               << " was stop by signal: " << WSTOPSIG(status);
  } else if (WIFCONTINUED(status)) {
    LOG(ERROR) << "process: " << pid() << " was continued by signal SIGCONT";
  } else {
    LOG(ERROR) << "process: " << pid() << " has unknown wait status";
  }
  return 0;
#elif defined(WIN32) || defined(WIN64)
  if (process_ == nullptr) {
    LOG(ERROR) << "failed to wait a process with pid: " << pid();
    return -1;
  }
  DWORD result = ::WaitForSingleObject(
      process_, std::max((DWORD)1, (DWORD)timestamp.milliseconds()));
  if (result != WAIT_OBJECT_0) return std::numeric_limits<int>::min();

  DWORD exit_code;
  if (!::GetExitCodeProcess(process_, &exit_code)) {
    LOG(ERROR) << "failed to get exit code for a process with id: " << pid();
  }
  return (int)exit_code;
#endif
}

uint64_t Process::CurrentProcessId() {
#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
  return static_cast<uint64_t>(::getpid());
#elif defined(WIN32) || defined(WIN64)
  return static_cast<uint64_t>(::GetCurrentProcessId());
#endif
}

uint64_t Process::ParentProcessId() {
#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
  return (uint64_t)::getppid();
#elif defined(WIN32) || defined(WIN64)
  DWORD current = ::GetCurrentProcessId();

  // Takes a snapshot of the specified processes
  PROCESSENTRY32 pe = {0};
  pe.dwSize = sizeof(PROCESSENTRY32);
  HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (hSnapshot == INVALID_HANDLE_VALUE) return (uint64_t)-1;

  // Smart resource cleaner pattern
  auto snapshot =
      ::resource(hSnapshot, [](HANDLE hObject) { ::CloseHandle(hObject); });

  // Retrieves information about all processes encountered in a system snapshot
  if (::Process32First(snapshot.get(), &pe)) {
    do {
      if (pe.th32ProcessID == current) {
        return (uint64_t)pe.th32ParentProcessID;
      }
    } while (::Process32Next(snapshot.get(), &pe));
  }

  return (uint64_t)-1;
#endif
}

void Process::Exit(int result) {
#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
  ::exit(result);
#elif defined(WIN32) || defined(WIN64)
  ::ExitProcess((UINT)result);
#endif
}

Process::Process(uint64_t pid) {
#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
  pid_ = (pid_t)pid;
#elif defined(WIN32) || defined(WIN64)
  pid_ = (DWORD)pid;
  process_ = ::OpenProcess(
      PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pid_);
  if (process_ == nullptr) {
    LOG(FATAL) << "failed to open a process with id: " << pid;
  }
#endif
}

#if defined(WIN32) || defined(WIN64)
Process::Process(DWORD pid, HANDLE process) {}
#endif

Process::~Process() {
#if defined(WIN32) || defined(_WIN64)
  if (process_ != nullptr) {
    if (!::CloseHandle(process_)) {
      LOG(FATAL) << "failed to open a process with id: " << pid;
    }
    process_ = nullptr;
  }
#endif
}

#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
static std::vector<char> PrepareEnvars(
    const std::map<std::string, std::string> *envars)
#elif defined(WIN32) || defined(WIN64)
static std::vector<wchar_t> PrepareEnvars(
    const std::map<std::string, std::string> *envars)
#endif
{
#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
  std::vector<char> result;
#elif defined(WIN32) || defined(_WIN64)
  std::vector<wchar_t> result;
#endif

  if (envars == nullptr) return result;

  for (const auto &envar : *envars) {
#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
    std::string key = envar.first;
    std::string value = envar.second;
#elif defined(WIN32) || defined(WIN64)
    std::wstring key = Encoding::FromUTF8(envar.first);
    std::wstring value = Encoding::FromUTF8(envar.second);
#endif
    result.insert(result.end(), key.begin(), key.end());
    result.insert(result.end(), '=');
    result.insert(result.end(), value.begin(), value.end());
    result.insert(result.end(), '\0');
  }
  result.insert(result.end(), '\0');

  return result;
}

// http://www.cnblogs.com/mickole/p/3188321.html
bool Process::Spwan(ProcessDesc &desc) {
  int channel[2];
  //   int ret, status;

  if (desc.restart_times_++ > 0) {
    if (!desc.flock_->LockAll()) {
      if (desc.pid_ > 0) {
        LOG(WARNING) << "kill old process, pid: " << desc.pid_;
        ::kill(desc.pid_, SIGKILL);
      }
    }
  }

  /* Making the second socket nonblocking is a bit subtle, given that we
   * ignore any EAGAIN returns when writing to it, and you don't usally
   * do that for a nonblocking socket. But if the kernel gives us EAGAIN,
   * then there's no need to add any more data to the buffer, since
   * the main thread is already either about to wake up and drain it,
   * or woken up and in the process of draining it.
   */
  if (!CreateSocketPair(channel)) {
    LOG(ERROR) << "failed to create pipe, errno: " << errno;
    return false;
  }

  // nolocking fd 0,1
  // 标记位, ioctl用于清除(0)或设置(非0)操作
  int on = 1;

  /* 设置channel[0]的信号驱动异步I/O标志
   * FIOASYNC：该状态标志决定是否收取针对socket的异步I/O信号（SIGIO）
   * 其与O_ASYNC文件状态标志等效，可通过fcntl的F_SETFL命令设置or清除
   */
  if (::ioctl(channel[0], FIOASYNC, &on) < 0) {
    LOG(ERROR) << "ioctl(FIOASYNC) failed while spawning";
    return false;
  }
  /* F_SETOWN：用于指定接收SIGIO和SIGURG信号的socket属主(进程ID或进程组ID)
   * 这里意思是指定Master进程接收SIGIO和SIGURG信号
   * SIGIO信号必须是在socket设置为信号驱动异步I/O才能产生,即上一步操作
   * SIGURG信号是在新的带外数据到达socket时产生的
   */
  if (::fcntl(channel[0], F_SETOWN, desc.pid_)) {
    LOG(ERROR) << "fcntl(F_SETOWN) failed while spawning";
    return false;
  }

  if (!SetCloseOnExec(channel[0])) {
    LOG(ERROR) << "fcntl(FD_CLOEXEC) failed while spawning";
    return false;
  }
  if (!SetCloseOnExec(channel[1])) {
    LOG(ERROR) << "fcntl(FD_CLOEXEC) failed while spawning";
    return false;
  }

  return true;
}

Process Process::Execute(const std::string &command,
                         const std::vector<std::string> *arguments,
                         const std::map<std::string, std::string> *envars,
                         const std::string *directory, int input, int output,
                         int error) {
#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
  // Prepare arguments
  size_t index = 0;
  std::vector<char *> argv(
      1 + ((arguments != nullptr) ? arguments->size() : 0) + 1);
  argv[index++] = (char *)command.c_str();
  if (arguments != nullptr) {
    for (const auto &argument : *arguments) {
      argv[index++] = (char *)argument.c_str();
    }
  }
  argv[index++] = nullptr;

  // Prepare environment variables
  std::vector<char> environment = PrepareEnvars(envars);

  // Fork the current process
  pid_t pid = ::fork();
  if (pid < 0) {
    LOG(FATAL) << "failed to fork the current process";
  } else if (pid == 0) {
    // Set environment variables of the new process
    if (!environment.empty()) {
      char *envar = environment.data();
      while (*envar != '\0') {
        ::putenv(envar);
        while (*envar != '\0') ++envar;
        ++envar;
      }
    }

    // Change the current directory of the new process
    if (directory != nullptr) {
      int result = ::chdir(directory->c_str());
      if (result != 0) ::_exit(666);
    }

    // Close pipes endpoints
    // Prepare input communication pipe
    if (input != -1) {
      ::dup2(input, STDIN_FILENO);
      ::close(input);
    }

    // Prepare output communication pipe
    if (output != -1) {
      ::dup2(output, STDOUT_FILENO);
      ::close(output);
    }

    // Prepare error communication pipe
    if (error != -1) {
      ::dup2(error, STDERR_FILENO);
      ::close(error);
    }

    // Close all open file descriptors other than stdin, stdout, stderr
    for (int i = 3; i < sysconf(_SC_OPEN_MAX); ++i) ::close(i);

    // Execute a new process image
    ::execvp(argv[0], argv.data());

    // Get here only if error occurred during image execution
    ::_exit(666);
  }

  // Return result process
  Process result(pid);
  return result;
#elif defined(WIN32) || defined(_WIN64)
  BOOL bInheritHandles = FALSE;

  // Prepare command line
  std::wstring command_line = Encoding::FromUTF8(command);
  if (arguments != nullptr) {
    for (const auto &argument : *arguments) {
      command_line.append(L" ");
      command_line.append(Encoding::FromUTF8(argument));
    }
  }

  // Prepare environment variables
  std::vector<wchar_t> environment = PrepareEnvars(envars);

  // Fill process startup information
  STARTUPINFOW si;
  ::GetStartupInfoW(&si);
  si.cb = sizeof(STARTUPINFOW);
  si.lpReserved = nullptr;
  si.lpDesktop = nullptr;
  si.lpTitle = nullptr;
  si.dwFlags = STARTF_FORCEOFFFEEDBACK;
  si.cbReserved2 = 0;
  si.lpReserved2 = nullptr;

  // Get the current process handle
  HANDLE hCurrentProcess = GetCurrentProcess();

  // Prepare input communication pipe
  if (input != nullptr) {
    ::DuplicateHandle(hCurrentProcess, input, hCurrentProcess, &si.hStdInput, 0,
                      TRUE, DUPLICATE_SAME_ACCESS);
  } else {
    HANDLE hStdHandle = ::GetStdHandle(STD_INPUT_HANDLE);
    if (hStdHandle != nullptr)
      ::DuplicateHandle(hCurrentProcess, hStdHandle, hCurrentProcess,
                        &si.hStdInput, 0, TRUE, DUPLICATE_SAME_ACCESS);
    else
      si.hStdInput = nullptr;
  }

  // Prepare output communication pipe
  if (output != nullptr) {
    ::DuplicateHandle(hCurrentProcess, output, hCurrentProcess, &si.hStdOutput,
                      0, TRUE, DUPLICATE_SAME_ACCESS);
  } else {
    HANDLE hStdHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdHandle != nullptr)
      ::DuplicateHandle(hCurrentProcess, hStdHandle, hCurrentProcess,
                        &si.hStdOutput, 0, TRUE, DUPLICATE_SAME_ACCESS);
    else
      si.hStdOutput = nullptr;
  }

  // Prepare error communication pipe
  if (error != nullptr) {
    ::DuplicateHandle(hCurrentProcess, error, hCurrentProcess, &si.hStdError, 0,
                      TRUE, DUPLICATE_SAME_ACCESS);
  } else {
    HANDLE hStdHandle = ::GetStdHandle(STD_ERROR_HANDLE);
    if (hStdHandle != nullptr)
      DuplicateHandle(hCurrentProcess, hStdHandle, hCurrentProcess,
                      &si.hStdError, 0, TRUE, DUPLICATE_SAME_ACCESS);
    else
      si.hStdError = nullptr;
  }

  // Close pipes endpoints
  if (input != nullptr) input->CloseRead();
  if (output != nullptr) output->CloseWrite();
  if (error != nullptr) error->CloseWrite();

  // Override standard communication pipes of the process
  if ((si.hStdInput != nullptr) || (si.hStdOutput != nullptr) ||
      (si.hStdError != nullptr)) {
    bInheritHandles = TRUE;
    si.dwFlags |= STARTF_USESTDHANDLES;
  }

  // Create a new process
  PROCESS_INFORMATION pi;
  if (!::CreateProcessW(
          nullptr, (wchar_t *)command_line.c_str(), nullptr, nullptr,
          bInheritHandles, CREATE_UNICODE_ENVIRONMENT,
          environment.empty() ? nullptr : (LPVOID)environment.data(),
          (!directory) ? nullptr : Encoding::FromUTF8(*directory).c_str(), &si,
          &pi)) {
    LOG(FATAL) << "failed to execute a new process with command: " << command;
  }

  // Close standard handles
  if (si.hStdInput != nullptr) ::CloseHandle(si.hStdInput);
  if (si.hStdOutput != nullptr) ::CloseHandle(si.hStdOutput);
  if (si.hStdError != nullptr) ::CloseHandle(si.hStdError);

  // Close thread handle
  ::CloseHandle(pi.hThread);

  pid = ::GetProcessId(pi.hProcess);
  if (!pid) {
    LOG(FATAL) << "could not get child process id.";
  }

  // Return result process
  Process result(pi.dwProcessId, pi.hProcess);
  return result;
#endif
}
}  // namespace MSF
