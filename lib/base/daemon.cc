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
#include "daemon.h"

#include <fcntl.h>
#include <signal.h>
#include <sysexits.h>
#include <sys/wait.h>

#include <butil/logging.h>

#include "utils.h"
#include "sock_utils.h"

using namespace MSF;

// #include <butil/logging.h>

namespace MSF {

// 参考lightttpd和nginx
// https://blog.csdn.net/weixin_40021744/article/details/105216611
// https://blog.csdn.net/fall221/article/details/45420197
// http://blog.chinaunix.net/uid-29482215-id-4123005.html
// glibc: daemon(0, 0)

/* if we want to ensure our ability to dump core, don't chdir to / */
void Daemonize(bool chdir, bool close) {
#if defined(HAVE_DAEMON) && !defined(__APPLE__) && !defined(__UCLIBC__)
  if (::daemon(true, false) == -1) {
    return;
  }
#else
  // 屏蔽一些有关控制终端操作的信号,
  // 防止守护进程没有正常运作之前控制终端受到干扰退出或挂起
  ::signal(SIGTTOU, SIG_IGN);  // 忽略后台进程写控制终端信号
  ::signal(SIGTTIN, SIG_IGN);  // 忽略后台进程读控制终端信号
  ::signal(SIGTSTP, SIG_IGN);  // 忽略终端挂起信号
  ::signal(SIGPIPE, SIG_IGN);  // 忽略终端挂起信号

  /* 从普通进程转换为守护进程
   * 步骤1: 后台运行
   * 操作1: 脱离控制终端->调用fork->终止父进程->子进程被init收养
   */
  int pipefd[2];
  pid_t pid;

  if (::pipe(pipefd) < 0) {
    // LOG(ERROR) << "pipe failed, exit";
    exit(EXIT_FAILURE);
  }

  /* this is loosely based off of glibc's daemon () implementation
   * http://sourceware.org/git/?p=glibc.git;a=blob_plain;f=misc/daemon.c */
  /* 1st fork detaches child from terminal */
  pid = ::fork();
  if (pid < 0) {
    LOG(ERROR) << "fork son process failed, exit";
    ::exit(EXIT_FAILURE);
  }
  if (pid > 0) {
    char buf;
    ssize_t bytes;

    ::close(pipefd[1]);
    /* parent waits for grandchild to be ready */
    do {
      bytes = ::read(pipefd[0], &buf, sizeof(buf));
    } while (bytes < 0 && EINTR == errno);
    ::close(pipefd[0]);

    if (bytes <= 0) {
      /* closed fd (without writing) == failure in grandchild */
      LOG(ERROR) << "failed to daemonize";
      ::exit(EXIT_FAILURE);
    }
    /* parent terminates */
    ::exit(EXIT_SUCCESS);
  }
  ::close(pipefd[0]);

  /* 步骤2: 脱离控制终端,登陆会话和进程组
   * 操作2: 使用setsid创建新会话,成为新会话的首进程,则与原来的
   *        登陆会话和进程组自动脱离,从而脱离控制终端.
   *       一步的fork保证了子进程不可能是一个会话的首进程,
   *        这是调用setsid的必要条件.
   */
  /* try to detach from parent's process group */
  /* 1st child continues and becomes the session and process group leader */
  if (::setsid() < 0) {
    // LOG(ERROR) << "setsid process failed, exit";
    ::exit(EXIT_FAILURE);
  }

  /* clear file mode creation mask */
  umask(0);

  ::signal(SIGCHLD, SIG_IGN);
  ::signal(SIGHUP, SIG_IGN);

  /* 步骤3: 孙子进程被托管给init进程
   *        上面已经完成了大部分工作, 但是有的系统上,当会话首进程打开
   *        一个尚未与任何会话关联的终端时,该设备自动作为控制终端分配给该会话
   *        为避免该情况,我们再次fork进程,于是新进程不再是会话首进程
   *        会话首进程退出时可能会给所有会话内的进程发送SIGHUP，而该
   *        信号默认是结束进程,故需要忽略该信号来防止孙子进程意外结束。
   */
  /* 2nd fork turns child into a non-session leader: cannot acquire terminal */
  pid = ::fork();
  if (pid < 0) {
    LOG(ERROR) << "fork grandson process failed, exit";
    ::exit(EXIT_FAILURE);
  }
  if (pid > 0) ::exit(EXIT_SUCCESS);

  /* 步骤4: 改变工作目录到根目录
   *       进程活动时,其工作目录所在的文件系统不能被umount
   */
  /* add option to change directory to root */
  if (chdir) {
    char path_dir[256] = {0};
    char *p = ::getcwd(path_dir, 256);
    MSF_UNUSED(p);
    if (::chdir(path_dir) != 0) {
      LOG(ERROR) << "daemon process chdir failed, exit";
      ::exit(EXIT_FAILURE);
    }
  }

  /* 步骤5: 重定向标准输入输出到/dev/null
   *        但是不关闭012的fd避免被分配
   */
  /* redirect stdin, stdout and stderr to "/dev/null" */
  if (close) {
    int fd = OpenDevNull(true);
    if (fd < 0) {
      LOG(ERROR) << "daemon process open /dev/null failed, exit";
      ::exit(EXIT_FAILURE);
    }
    /* Redirect standard files to /dev/null */
    // if (freopen("/dev/null", "r", stdin) == NULL)
    //   std::exit(EXIT_FAILURE);
    // if (freopen("/dev/null", "w", stdout) == NULL)
    //   std::exit(EXIT_FAILURE);
    // if (freopen("/dev/null", "w", stderr) == NULL)
    //   std::exit(EXIT_FAILURE);

    if (::dup2(fd, STDIN_FILENO) < 0) {
      LOG(ERROR) << "daemon process ::dup2 stdin failed, exit";
      ::exit(EXIT_FAILURE);
    }
    if (::dup2(fd, STDOUT_FILENO) < 0) {
      LOG(ERROR) << "daemon process ::dup2 stdout failed, exit";
      ::exit(EXIT_FAILURE);
    }
    // if (::dup2(fd, STDERR_FILENO) < 0) {
    // LOG(ERROR) << "daemon process ::dup2 stderr failed, exit";
    //   exit(EXIT_FAILURE);
    // }

    if (fd > STDERR_FILENO) {
      if (::close(fd) < 0) {
        LOG(ERROR) << "daemon process close fd failed, exit";
        ::exit(EXIT_FAILURE);
      }
    }
  }

  /**
   * notify daemonize-grandparent of successful startup
   * do this before any further forking is done (workers)
   */
  if (::write(pipefd[1], "", 1) < 0) {
    ::exit(EXIT_FAILURE);
  }
  ::close(pipefd[1]);
#endif
}

void ForkExecve() {
  Daemonize();
  // execve(name, argv, envp ? envp : environ);
}

int childpid = -1;
volatile sig_atomic_t killed_by_us = 0;
volatile sig_atomic_t fatal_error_in_progress = 0;

static void termination_handler(int sig);
static void termination_handler(int sig) {
  int old_errno;

  if (fatal_error_in_progress) {
    raise(sig);
  }
  fatal_error_in_progress = 1;

  if (childpid > 0) {
    old_errno = errno;
    /* we were killed (SIGTERM), so make sure child dies too */
    // kill_process_group();
    errno = old_errno;
  }

  ::signal(sig, SIG_DFL);
  raise(sig);
}

void install_termination_handler(void);
void install_termination_handler(void) {
  struct sigaction sa, old_sa;
  sa.sa_handler = termination_handler;
  sigemptyset(&sa.sa_mask);
  sigaddset(&sa.sa_mask, SIGINT);
  sigaddset(&sa.sa_mask, SIGHUP);
  sigaddset(&sa.sa_mask, SIGTERM);
  sigaddset(&sa.sa_mask, SIGQUIT);
  sa.sa_flags = 0;
  sigaction(SIGINT, NULL, &old_sa);
  if (old_sa.sa_handler != SIG_IGN) sigaction(SIGINT, &sa, NULL);
  sigaction(SIGHUP, NULL, &old_sa);
  if (old_sa.sa_handler != SIG_IGN) sigaction(SIGHUP, &sa, NULL);
  sigaction(SIGTERM, NULL, &old_sa);
  if (old_sa.sa_handler != SIG_IGN) sigaction(SIGTERM, &sa, NULL);
}
void KillGroupProcess(int childpid) {
  int pgid = ::getpgid(childpid);
  killed_by_us = 1;
  if (::killpg(pgid, SIGTERM) < 0) {
    perror("killpg");
    ::exit(EX_OSERR);
  }
}

bool RunSubprocess(char *command, char **args,
                   void (*pre_wait_function)(void)) {
  int pid;
  int status;

  int childpid = ::fork();
  if (childpid == 0) {
    /* try to detach from parent's process group */
    if (::setsid() == -1) {
      LOG(ERROR) << "unable to detach child process, aborting";
      return -1;
    }
    if (::execvp(command, args)) {
      perror("execvp");
      ::exit(EX_NOINPUT);
    } else {
      /* Control almost certainly will not get to this point, ever. If
       * the call to execvp returned, instead of switching to a new memory
       * image, there was a problem. This exit will be collected by the
       * parent's call to waitpid() below.
       */
      ::exit(EX_UNAVAILABLE);
    }
  } else if (childpid < 0) {
    perror("fork");
    ::exit(EX_OSERR);
  } else {
    /* Make sure the child dies if we get killed. */
    /* Only the parent should do this, of course! */
    install_termination_handler();

    if (pre_wait_function != NULL) {
      pre_wait_function();
    }

    /* blocking wait on the child */
    while ((pid = ::waitpid(childpid, &status, 0)) < 0) {
      if (errno == EINTR) {
        if (killed_by_us) {
          break;
        } /* else restart the loop */
      } else {
        perror("waitpid");
      }
    }
    alarm(0);
    if (pid > 0) {
      if (pid != childpid) {
        LOG(ERROR) << "childpid " << childpid
                   << " not retured by waitpid! instead ";
        KillGroupProcess(childpid);
        ::exit(EX_OSERR);
      }

      /* exited normally? */
      if (WIFEXITED(status)) {
        /* decode and return exit sttus */
        status = WEXITSTATUS(status);
        LOG(ERROR) << "child exited with status " << status;
      } else {
        /* This formula is a Unix shell convention */
        status = 128 + WTERMSIG(status);
        LOG(ERROR) << "child exited via ::signal " << WTERMSIG(status);
      }
    }
    childpid = -1;
  }
  return status;
}

}  // namespace MSF