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
#include <base/Daemon.h>
#include <base/Logger.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

namespace MSF {
namespace BASE {

#ifndef O_BINARY
#define O_BINARY 0
#endif
#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif
#ifndef O_NOCTTY
#define O_NOCTTY 0
#endif
#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif

/*(O_NOFOLLOW is not handled here)*/
/*(Note: O_NOFOLLOW affects only the final path segment, the target file,
 * not any intermediate symlinks along the path)*/

/* O_CLOEXEC handled further below, if defined) */
#ifdef O_NONBLOCK
#define FD_O_FLAGS (O_BINARY | O_LARGEFILE | O_NOCTTY | O_NONBLOCK)
#else
#define FD_O_FLAGS (O_BINARY | O_LARGEFILE | O_NOCTTY)
#endif

int OpenExec(const char *pathname, bool symlinks, int flags, mode_t mode) {
  if (!symlinks) flags |= O_NOFOLLOW;

#ifdef O_CLOEXEC
  return open(pathname, flags | O_CLOEXEC | FD_O_FLAGS, mode);
#else
  int fd = open(pathname, flags | FDEVENT_O_FLAGS, mode);
#ifdef FD_CLOEXEC
  if (fd > 0) {
    fcntl(fd, F_SETFD, FD_CLOEXEC);
  }
#endif
  return fd;
#endif
}

int OpenDevNull(void) {
#if defined(_WIN32_)
  return OpenExec("nul", 0, O_RDWR, 0);
#else
  return OpenExec("/dev/null", false, O_RDWR, 0);
#endif
}

// 参考lightttpd和nginx
// https://blog.csdn.net/weixin_40021744/article/details/105216611
// https://blog.csdn.net/fall221/article/details/45420197
// http://blog.chinaunix.net/uid-29482215-id-4123005.html
// glibc: daemon(0, 0)

/* if we want to ensure our ability to dump core, don't chdir to / */
void daemonize(bool chdir, bool close) {
  // 屏蔽一些有关控制终端操作的信号,
  // 防止守护进程没有正常运作之前控制终端受到干扰退出或挂起
  signal(SIGTTOU, SIG_IGN);  // 忽略后台进程写控制终端信号
  signal(SIGTTIN, SIG_IGN);  // 忽略后台进程读控制终端信号
  signal(SIGTSTP, SIG_IGN);  // 忽略终端挂起信号

  /* 从普通进程转换为守护进程
   * 步骤1: 后台运行
   * 操作1: 脱离控制终端->调用fork->终止父进程->子进程被init收养
   */
  int pipefd[2];
  pid_t pid;

  if (pipe(pipefd) < 0) {
    MSF_ERROR << "pipe failed, exit";
    exit(EXIT_FAILURE);
  }

  pid = fork();
  if (pid < 0) {
    MSF_ERROR << "fork son process failed, exit";
    exit(EXIT_FAILURE);
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
      MSF_ERROR << "failed to daemonize";
      exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
  }
  ::close(pipefd[0]);

  /* 步骤2: 脱离控制终端,登陆会话和进程组
   * 操作2: 使用setsid创建新会话,成为新会话的首进程,则与原来的
   *        登陆会话和进程组自动脱离,从而脱离控制终端.
   *       一步的fork保证了子进程不可能是一个会话的首进程,
   *        这是调用setsid的必要条件.
   */
  if (setsid() < 0) {
    MSF_ERROR << "setsid process failed, exit";
    exit(EXIT_FAILURE);
  }

  /* set files mask */
  umask(0);

  signal(SIGCHLD, SIG_IGN);
  signal(SIGHUP, SIG_IGN);

  /* 步骤3: 孙子进程被托管给init进程
   *        上面已经完成了大部分工作, 但是有的系统上,当会话首进程打开
   *        一个尚未与任何会话关联的终端时,该设备自动作为控制终端分配给该会话
   *        为避免该情况,我们再次fork进程,于是新进程不再是会话首进程
   *        会话首进程退出时可能会给所有会话内的进程发送SIGHUP，而该
   *        信号默认是结束进程,故需要忽略该信号来防止孙子进程意外结束。
   */
  pid = fork();
  if (pid < 0) {
    MSF_ERROR << "fork grandson process failed, exit";
    exit(EXIT_FAILURE);
  }
  if (pid > 0) exit(EXIT_SUCCESS);

  /* 步骤4: 改变工作目录到根目录
   *       进程活动时,其工作目录所在的文件系统不能被umount
   */
  if (chdir) {
    char path_dir[0x100] = {0};
    getcwd(path_dir, 0x100);
    if (::chdir(path_dir) != 0) {
      MSF_ERROR << "daemon process chdir failed, exit";
      exit(EXIT_FAILURE);
    }
  }

  /* 步骤5: 重定向标准输入输出到/dev/null
   *        但是不关闭012的fd避免被分配
   */
  if (close) {
    int fd = OpenDevNull();
    if (fd < 0) {
      MSF_ERROR << "daemon process open /dev/null failed, exit";
      exit(EXIT_FAILURE);
    }

    if (dup2(fd, STDIN_FILENO) < 0) {
      MSF_ERROR << "daemon process dup2 stdin failed, exit";
      exit(EXIT_FAILURE);
    }
    if (dup2(fd, STDOUT_FILENO) < 0) {
      MSF_ERROR << "daemon process dup2 stdout failed, exit";
      exit(EXIT_FAILURE);
    }
    // if (dup2(fd, STDERR_FILENO) < 0) {
    //   MSF_ERROR << "daemon process dup2 stderr failed, exit";
    //   exit(EXIT_FAILURE);
    // }

    if (fd > STDERR_FILENO) {
      if (::close(fd) < 0) {
        MSF_ERROR << "daemon process close fd failed, exit";
        exit(EXIT_FAILURE);
      }
    }
  }

  /**
   * notify daemonize-grandparent of successful startup
   * do this before any further forking is done (workers)
   */
  if (::write(pipefd[1], "", 1) < 0) {
    exit(EXIT_FAILURE);
  }
  ::close(pipefd[1]);
}

}  // namespace BASE
}  // namespace MSF