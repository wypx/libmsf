#include <assert.h>
#include <dirent.h>
#include <pwd.h>
#include <stdio.h>  // snprintf
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <unistd.h>
#include <algorithm>
#include <butil/logging.h>

#include "Process.h"


using namespace MSF;

/* 其他方法:
 * 1. /proc/pid/status
 * 2. 进程名字既可以指向argv[0]你,
 *    也可以阅读/proc/self/status.
 *    或者你可以使用getenv("_"),
 *    不确定是谁设置的,​​以及它的可靠性.
 * 3. 如果你使用glibc，那么：
 *    #define _GNU_SOURCE
 *    #include <errno.h>
 *    extern char *program_invocation_name;
 *    extern char *program_invocation_short_name;
 *    在大多数Unices下,__progname也由libc定义.
 *    唯一便携的方式是使用argv[0]
 * */

/*  获取进程pid
    FILE *fp = popen("ps -e | grep \'test\' | awk \'{print $1}\'", "r");
    char buffer[10] = {0};
    while (NULL != fgets(buffer, 10, fp)) //逐行读取执行结果并打印
    {
        printf("PID:  %s", buffer);
    }
    pclose(fp);
*/

#if 0
namespace MSF {


__thread int t_numOpenedFiles = 0;
int fdDirFilter(const struct dirent* d)
{
    if (::isdigit(d->d_name[0]))
    {
        ++t_numOpenedFiles;
    }
    return 0;
}

__thread std::vector<pid_t>* t_pids = NULL;
int taskDirFilter(const struct dirent* d)
{
    if (::isdigit(d->d_name[0]))
    {
        t_pids->push_back(atoi(d->d_name));
    }
    return 0;
}

int scanDir(const char *dirpath, int (*filter)(const struct dirent *))
{
    struct dirent** namelist = NULL;
    int result = ::scandir(dirpath, &namelist, filter, alphasort);
    assert(namelist == NULL);
    return result;
}

// Timestamp g_startTime = Timestamp::now();
// assume those won't change during the life time of a process.
int g_clockTicks = static_cast<int>(::sysconf(_SC_CLK_TCK));
int g_pageSize = static_cast<int>(::sysconf(_SC_PAGE_SIZE));

pid_t Process::pid()
{
  return ::getpid();
}

std::string Process::pidString()
{
    return std::string(pid());
}

uid_t Process::uid()
{
  return ::getuid();
}

string Process::username()
{
    struct passwd pwd;
    struct passwd* result = NULL;
    char buf[8192];
    const char* name = "unknownuser";

    getpwuid_r(uid(), &pwd, buf, sizeof buf, &result);
    if (result)
    {
        name = pwd.pw_name;
    }
    return name;
}

uid_t Process::euid()
{
    return ::geteuid();
}

// Timestamp Process::startTime()
// {
//     return g_startTime;
// }

int Process::clockTicksPerSecond()
{
    return g_clockTicks;
}

int Process::pageSize()
{
    return g_pageSize;
}

bool Process::isDebugBuild()
{
#ifdef NDEBUG
    return false;
#else
    return true;
#endif
}

std::string Process::hostname()
{
  // HOST_NAME_MAX 64
  // _POSIX_HOST_NAME_MAX 255
    char buf[256];
    if (::gethostname(buf, sizeof buf) == 0)
    {
        buf[sizeof(buf)-1] = '\0';
        return buf;
    }
    else
    {
        return "unknownhost";
    }
}

std::string Process::procname()
{
    return procname(procStat()).as_string();
}

// StringPiece Process::procname(const std::string& stat)
// {
//   StringPiece name;
//   size_t lp = stat.find('(');
//   size_t rp = stat.rfind(')');
//   if (lp != string::npos && rp != string::npos && lp < rp)
//   {
//     name.set(stat.data()+lp+1, static_cast<int>(rp-lp-1));
//   }
//   return name;
// }

std::string Process::procStatus()
{
    // std::string result;
    // FileUtil::readFile("/proc/self/status", 65536, &result);
    // return result;
}

std::string Process::procStat()
{
    // string result;
    // FileUtil::readFile("/proc/self/stat", 65536, &result);
    // return result;
}

std::string Process::threadStat()
{
    // char buf[64];
    // snprintf(buf, sizeof buf, "/proc/self/task/%d/stat", CurrentThread::tid());
    // string result;
    // FileUtil::readFile(buf, 65536, &result);
    // return result;
}

std::string Process::exePath()
{
    std::string result;
    char buf[1024];
    ssize_t n = ::readlink("/proc/self/exe", buf, sizeof buf);
    if (n > 0)
    {
        result.assign(buf, n);
    }
    return result;
}

int Process::openedFiles()
{
    t_numOpenedFiles = 0;
    scanDir("/proc/self/fd", fdDirFilter);
    return t_numOpenedFiles;
}

int Process::maxOpenFiles()
{
    struct rlimit rl;
    if (::getrlimit(RLIMIT_NOFILE, &rl))
    {
        return openedFiles();
    }
    else
    {
        return static_cast<int>(rl.rlim_cur);
    }
}

Process::CpuTime Process::cpuTime()
{
    Process::CpuTime t;
    struct tms tms;
    if (::times(&tms) >= 0) {
        const double hz = static_cast<double>(clockTicksPerSecond());
        t.userSeconds = static_cast<double>(tms.tms_utime) / hz;
        t.systemSeconds = static_cast<double>(tms.tms_stime) / hz;
    }
    return t;
}

int Process::numThreads()
{
    int result = 0;
    std::string status = procStatus();
    size_t pos = status.find("Threads:");
    if (pos != string::npos)
    {
        result = ::atoi(status.c_str() + pos + 8);
    }
    return result;
}

std::vector<pid_t> Process::threads()
{
    std::vector<pid_t> result;
    t_pids = &result;
    scanDir("/proc/self/task", taskDirFilter);
    t_pids = NULL;
    std::sort(result.begin(), result.end());
    return result;
}


void process_wait_child_termination(pid_t v_pid)
{
    pid_t pid;
    int status;

    /*wait不能使用在while中,因为wait是阻塞的,比如当某子进程一直不停止，
    它将一直阻塞,从而影响了父进程.于是使用waitpid，options设置为WNOHANG,
    可以在非阻塞情况下处理多个退出子进程*/
    while ((pid = waitpid(v_pid, &status, WNOHANG))) {
        if (pid == -1)  {
            if (errno == EINTR)
                continue;
            else
                break;
        }
    }

    if (WIFEXITED(status)) {
        LOG(INFO) << "Child " << v_pid << " normal exit " << WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        LOG(INFO) << "Child abnormally exit, signal " << WTERMSIG(status);
    }
}

//http://www.cnblogs.com/mickole/p/3188321.html
int process_spwan_grandson(int wfd, struct process_desc *proc_desc) {

    pid_t grandson;

    grandson = fork();
    if (grandson > 0) {
        printf("Now child process exit, pid(%d).\n", getpid());
        /*子进程退出,孙子进程让init进程托管,避免僵尸进程*/
        exit(0);
    } else if (0 == grandson) {

        printf("Now grandson process(%s) pid(%d) enter.\n", 
        proc_desc->proc_name, getpid());

        pid_t pid = getpid();
        while (send(wfd, (char*)&pid, sizeof(pid), MSG_NOSIGNAL) != sizeof(pid_t));

        char *argv[4] = { proc_desc->proc_path, "-c", proc_desc->proc_conf, NULL };
        // 数组最后一位需要为0
        execvp(proc_desc->proc_path, argv);

        //execvp
        //exec系统调用会从当前进程中把当前程序的机器指令清除，
        //然后在空的进程中载入调用时指定的程序代码
        execv(proc_desc->proc_path, argv);
        printf("Now grandson process pid(%d) exit.\n", getpid());
        exit(-1);
    }

    return getpid();
}

int process_spwan(struct process_desc *proc_desc) {

        int rc;
        int fd[2];
        pid_t child;

        pid_t exit_child;
        int status;

#if 0
        if (proc_desc->restart_times++ > 0) {
            rc = process_try_lock(proc_desc->proc_name, 0);
            if (false == rc) {
                if (proc_desc->proc_pid > 0) {
                    printf("Now kill old process, pid(%d).\n", proc_desc->proc_pid);
                    kill(proc_desc->proc_pid, SIGKILL);
                }
            }
        }
#endif

    /*
      Making the second socket nonblocking is a bit subtle, given that we
      ignore any EAGAIN returns when writing to it, and you don't usally
      do that for a nonblocking socket. But if the kernel gives us EAGAIN,
      then there's no need to add any more data to the buffer, since
      the main thread is already either about to wake up and drain it,
      or woken up and in the process of draining it.
    */

    //socketpair(AF_UNIX, SOCK_STREAM, 0, ngx_processes[s].channel)

    //rc = pipe(fd);
    rc = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
    if (rc < 0) {  
        perror ("pipe error\n");  
        return -1;  
    }

    //nolocking fd 0,1
#if 0
	 /* 
            设置异步模式： 这里可以看下《网络编程卷一》的ioctl函数和fcntl函数 or 网上查询 
          */  
        on = 1; // 标记位，ioctl用于清除（0）或设置（非0）操作  

        /* 
          设置channel[0]的信号驱动异步I/O标志 
          FIOASYNC：该状态标志决定是否收取针对socket的异步I/O信号（SIGIO） 
          其与O_ASYNC文件状态标志等效，可通过fcntl的F_SETFL命令设置or清除 
         */ 
        if (ioctl(ngx_processes[s].channel[0], FIOASYNC, &on) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "ioctl(FIOASYNC) failed while spawning \"%s\"", name);
            ngx_close_channel(ngx_processes[s].channel, cycle->log);
            return NGX_INVALID_PID;
        }

        /* F_SETOWN：用于指定接收SIGIO和SIGURG信号的socket属主（进程ID或进程组ID） 
          * 这里意思是指定Master进程接收SIGIO和SIGURG信号 
          * SIGIO信号必须是在socket设置为信号驱动异步I/O才能产生，即上一步操作 
          * SIGURG信号是在新的带外数据到达socket时产生的 
         */ 
        if (fcntl(ngx_processes[s].channel[0], F_SETOWN, ngx_pid) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "fcntl(F_SETOWN) failed while spawning \"%s\"", name);
            ngx_close_channel(ngx_processes[s].channel, cycle->lo;
            return NGX_INVALID_PID;
        }
            
        
        /* FD_CLOEXEC：用来设置文件的close-on-exec状态标准 
          *             在exec()调用后，close-on-exec标志为0的情况下，此文件不被关闭；非零则在exec()后被关闭 
          *             默认close-on-exec状态为0，需要通过FD_CLOEXEC设置 
          *     这里意思是当Master父进程执行了exec()调用后，关闭socket        
          */
        if (fcntl(ngx_processes[s].channel[0], F_SETFD, FD_CLOEXEC) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "fcntl(FD_CLOEXEC) failed while spawning \"%s\"",
                           name);
            ngx_close_channel(ngx_processes[s].channel, cycle->log);
            return NGX_INVALID_PID;
        }

        
        /* 同上，这里意思是当Worker子进程执行了exec()调用后，关闭socket */  
        if (fcntl(ngx_processes[s].channel[1], F_SETFD, FD_CLOEXEC) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "fcntl(FD_CLOEXEC) failed while spawning \"%s\"",
                           name);
            ngx_close_channel(ngx_processes[s].channel, cycle->log);
            return NGX_INVALID_PID;
        }
#endif

    child = fork();
    if (child > 0) {
        close(fd[1]);
        proc_desc->proc_fd = fd[0];

        while (recv(fd[0], &proc_desc->proc_pid, 
            sizeof(pid_t), 0) != sizeof(pid_t));

        process_wait_child_termination(child);

        printf("Parent recv real %s pid(%d).\n",  proc_desc->proc_name, proc_desc->proc_pid);

        return proc_desc->proc_pid;	

    } else if (0 == child) {

            printf("Child name %s pid(%d).\n",  proc_desc->proc_name, getpid());

            close(fd[0]);

            /* set files mask */
            umask(0);
            if (setsid() == -1) exit(0);

            /* Request SIGTERM if parent dies */
            // prctl(PR_SET_PDEATHSIG, SIGTERM);
            /* Parent died already? */
            // if (getppid() == 1)
            //  ::kill(getpid(), SIGTERM);//SIGKILL
            signal(SIGHUP, SIG_IGN);
            signal(SIGTTOU, SIG_IGN); //忽略后台进程写控制终端信号
            signal(SIGTTIN, SIG_IGN); //忽略后台进程读控制终端信号
            signal(SIGTSTP, SIG_IGN); //忽略终端挂起

            process_spwan_grandson(fd[1], proc_desc);

        } else {
            perror("fork error\n");  
        }

        return 0;
}

/* read_pid
 *
 * Reads the specified pidfile and returns the read pid.
 * 0 is returned if either there's no pidfile, it's empty
 * or no pid can be read.
 */
pid_t read_pid (const char *pidfile)
{
  FILE *f;
  long pid;

  if (!(f=fopen(pidfile,"r")))
    return 0;
  if(fscanf(f,"%20ld", &pid) != 1)
    pid = 0;
  fclose(f);
  return (pid_t)pid;
}

/* check_pid
 *
 * Reads the pid using read_pid and looks up the pid in the process
 * table (using /proc) to determine if the process already exists. If
 * so the pid is returned, otherwise 0.
 */
pid_t check_pid (const char *pidfile)
{
  pid_t pid = read_pid(pidfile);

  /* Amazing ! _I_ am already holding the pid file... */
  if ((!pid) || (pid == getpid ()))
    return 0;

  /*
   * The 'standard' method of doing this is to try and do a 'fake' kill
   * of the process.  If an ESRCH error is returned the process cannot
   * be found -- GW
   */
  /* But... errno is usually changed only on error.. */
  errno = 0;
  if (kill(pid, 0) && errno == ESRCH)
	  return 0;

  return pid;
}

/* write_pid
 *
 * Writes the pid to the specified file. If that fails 0 is
 * returned, otherwise the pid.
 */
pid_t write_pid (const char *pidfile)
{
  FILE *f;
  int fd;
  pid_t pid;

  if ((fd = open(pidfile, O_RDWR|O_CREAT, 0644)) == -1) {
      return 0;
  }

  if ((f = fdopen(fd, "r+")) == NULL) {
      close(fd);
      return 0;
  }
  
#ifdef HAVE_FLOCK
  if (flock(fd, LOCK_EX|LOCK_NB) == -1) {
      fclose(f);
      return 0;
  }
#endif

  pid = getpid();
  if (!fprintf(f,"%ld\n", (long)pid)) {
      fclose(f);
      return 0;
  }
  fflush(f);

#ifdef HAVE_FLOCK
  if (flock(fd, LOCK_UN) == -1) {
      fclose(f);
      return 0;
  }
#endif
  fclose(f);

  return pid;
}

/* remove_pid
 *
 * Remove the the specified file. The result from unlink(2)
 * is returned
 */
int remove_pid (const char *pidfile)
{
  return unlink (pidfile);
}

static void consider_pidfile() {
    int pid=0;
    int i,l;
    char buf[80];

    /* Read previous pid file. */
    i = open("pid_path",O_RDONLY);
    if (i < 0) {
        /* l2tp_log(LOG_DEBUG, "%s: Unable to read pid file [%s]\n",
           __FUNCTION__, gconfig.pidfile);
         */
    } else
    {
        l=read(i,buf,sizeof(buf)-1);
        close (i);
        if (l >= 0)
        {
            buf[l] = '\0';
            pid = atoi(buf);
        }

        /* If the previous server process is still running,
           complain and exit immediately. */
        if (pid && pid != getpid () && kill (pid, 0) == 0)
        {
            // l2tp_log(LOG(INFO),
            //         "%s: There's already a xl2tpd server running.\n",
            //         __FUNCTION__);
            // close(server_socket);
            exit(1);
        }
    }

    pid = setsid();

    unlink(gconfig.pidfile);
    if ((i = open ("pid_file", O_WRONLY | O_CREAT, 0640)) >= 0) {
        snprintf (buf, sizeof(buf), "%d\n", (int)getpid());
        if (-1 == write (i, buf, strlen(buf)))
        {
            // l2tp_log (LOG_CRIT, "%s: Unable to write to %s.\n",
            //         __FUNCTION__, gconfig.pidfile);
            close (i);
            exit(1);
        }
        close (i);
    }
}



}
#endif