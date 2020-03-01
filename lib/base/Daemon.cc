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

int MSF_DAEMON::msf_redirect_output(const char *dev, int *fd_output,
                                    int *fd_stdout) {
  if (!dev || !fd_output || !fd_stdout) {
    return MSF_ERR;
  }

  if (*fd_output >= 0 || *fd_stdout >= 0) {
    return MSF_ERR;
  }

  *fd_output = open(dev, O_WRONLY | O_NONBLOCK);
  if (*fd_output < 0) {
    return MSF_ERR;
  }

  *fd_stdout = dup(STDOUT_FILENO);
  if (*fd_stdout < 0) {
    // MSF_UTILS_LOG(DBG_ERROR, "err in dup STDOUT.");
    close(*fd_output);
    *fd_output = MSF_INVALID_SOCKET;
    return MSF_ERR;
  }

  if (dup2(*fd_output, STDOUT_FILENO) < 0) {
    // MSF_UTILS_LOG(DBG_ERROR, "err in dup STDOUT.");
    close(*fd_output);
    *fd_output = MSF_INVALID_SOCKET;
    *fd_stdout = MSF_INVALID_SOCKET;
    return MSF_ERR;
  }

  if (dup2(*fd_output, STDERR_FILENO) < 0) {
    // MSF_UTILS_LOG(DBG_ERROR, "err in dup STDERR.");
    close(*fd_output);
    *fd_output = MSF_INVALID_SOCKET;
    *fd_stdout = MSF_INVALID_SOCKET;
    return MSF_ERR;
  }
  return MSF_OK;
}

int MSF_DAEMON::msf_redirect_output_cancle(int *fd_output, int *fd_stdout) {
  if (*fd_output < 0 || *fd_stdout < 0) {
    return MSF_ERR;
  }

  if (dup2(*fd_output, STDOUT_FILENO) < 0) {
    // MSF_UTILS_LOG(DBG_ERROR, "<rdirection_output_cancle> err in cancle
    // redirection STDOUT.\n");
    return MSF_ERR;
  }
  if (dup2(*fd_stdout, STDERR_FILENO) < 0) {
    // MSF_UTILS_LOG(DBG_ERROR, "<rdirection_output_cancle> err in cancle
    // redirection STDERR.\n");
    return MSF_ERR;
  }

  close(*fd_output);
  close(*fd_stdout);

  return MSF_OK;
}

/* if we want to ensure our ability to dump core, don't chdir to / */
int MSF_DAEMON::msf_daemonize(bool nochdir, bool noclose) {
  int fd = MSF_INVALID_SOCKET;

#ifdef SIGTTOU
  /* 下面用于屏蔽一些有关控制终端操作的信号
   * 防止守护进程没有正常运作之前控制终端受到干扰退出或挂起 */
  signal(SIGTTOU, SIG_IGN);  //忽略后台进程写控制终端信号
#endif

#ifdef SIGTTIN
  signal(SIGTTIN, SIG_IGN);  //忽略后台进程读控制终端信号
#endif

#ifdef SIGTSTP
  signal(SIGTSTP, SIG_IGN);  //忽略终端挂起
#endif

#ifndef USE_DAEMON

  /* 下面开始从普通进程转换为守护进程
   * 目标: 后台运行。
   * 做法:   脱离控制终端->调用fork->终止父进程->子进程被init收养.
   *
   * 目标: 脱离控制终端, 登陆会话和进程组
   * 做法: 使用setsid创建新会话,成为新会话的首进程,则与原来的
   *       登陆会话和进程组自动脱离,从而脱离控制终端.
   *       一步的fork保证了子进程不可能是一个会话的首进程,
   *       这是调用setsid的必要条件.
   */

  int pipefd[2];
  pid_t pid;

  if (pipe(pipefd) < 0) exit(-1);

  if (0 > (pid = fork())) exit(-1);

  if (pid > 0) {
    char buf;
    ssize_t bytes;

    close(pipefd[1]);
    /* parent waits for grandchild to be ready */
    do {
      bytes = read(pipefd[0], &buf, sizeof(buf));
    } while (bytes < 0 && EINTR == errno);
    close(pipefd[0]);

    if (bytes <= 0) {
      /* closed fd (without writing) == failure in grandchild */
      // MSF_UTILS_LOG(DBG_ERROR, "Failed to daemonize.");
      exit(-1);
    }

    exit(0);
  }
  close(pipefd[0]);

  /* set files mask */
  umask(0);

  if (setsid() == -1) exit(0);

  signal(SIGHUP, SIG_IGN);

  if (0 != fork()) exit(0);

  /* 上面已经完成了大部分工作，但是有的系统上，当会话首进程打开
   * 一个尚未与任何会话相关联的终端设备时，该设备自动作为控制
   * 终端分配给该会话。
   * 为避免该情况，我们再次fork进程，于是新进程不再是会话首进程。
   * 会话首进程退出时可能会给所有会话内的进程发送SIGHUP，而该
   * 信号默认是结束进程，故需要忽略该信号来防止孙子进程意外结束。
   */

  /* 最后目标：改变工作目录到根目录。
   * 原因：进程活动时，其工作目录所在的文件系统不能卸下。
   */

  if (nochdir == 0) {
    if (chdir("/") != 0) {
      // perror("chdir");
      exit(EXIT_FAILURE);
    }
  }

  // fclose(stderr);
  // fclose(stdout);
  if (noclose == 0 && (fd = open("/dev/null", O_RDWR, 0)) != -1) {
    if (dup2(fd, STDIN_FILENO) < 0) {
      // MSF_UTILS_LOG(DBG_ERROR, "dup2 stdin");
      return MSF_ERR;
    }
    if (dup2(fd, STDOUT_FILENO) < 0) {
      // MSF_UTILS_LOG(DBG_ERROR, "dup2 stdout");
      return MSF_ERR;
    }
    if (dup2(fd, STDERR_FILENO) < 0) {
      // MSF_UTILS_LOG(DBG_ERROR, "dup2 stderr");
      return MSF_ERR;
    }

    if (fd > STDERR_FILENO) {
      if (close(fd) < 0) {
        // MSF_UTILS_LOG(DBG_ERROR, "close fd failed");
        return MSF_ERR;
      }
    }
  }
#else
  if (daemon(0, 0) < 0) {
    MSF_UTILS_LOG(DBG_ERROR, "daemon()");
  }
#endif
  return MSF_OK;
}

void MSF_DAEMON::msf_close_std_fds(void) {
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
}

}  // namespace BASE
}  // namespace MSF