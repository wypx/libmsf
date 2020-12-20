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
#include "signal_manager.h"

#include <butil/logging.h>

#include "Os.h"

using namespace MSF;

namespace MSF {

void SignalHandler(int signo) {
  OsInfo os;
  //如果是日志进程的话，需要刷新buff到文件或者屏幕
  switch (signo) {
    case SIGBUS:
      LOG(ERROR) << "Got sigbus error.";
      raise(SIGKILL);
      break;
    case SIGSEGV:
      LOG(ERROR) << "Got sigsegv error.";
      break;
    case SIGILL:
      LOG(ERROR) << "Got sigill error.";
      break;
    case SIGCHLD:
    default:
      LOG(ERROR) << "Got sigill: " << signo;
      break;
  }
  os.writeStackTrace("/home/luotang.me/trace/trace.log");
  exit(1);
}

void RegSigHandler(int sig, SigHandler handler) {
  struct sigaction action;
  action.sa_handler = handler;
  sigemptyset(&action.sa_mask);
  /* 安装信号的时候, 设置 SA_RESTART属性,
   * 那么当信号处理函数返回后, 被该信号中断的系统调用将自动恢复*/
  action.sa_flags = SA_NODEFER | SA_RESTART;
  sigaction(sig, &action, nullptr);
}

void InitSigHandler() {
  /* interrupt, generated from terminal special char */
  RegSigHandler(SIGINT,
                SignalHandler);  //当按下ctrl+c时，它的效果就是发送SIGINT信号
  RegSigHandler(SIGPIPE, SIG_IGN);
  /* hangup, generated when terminal disconnects */
  RegSigHandler(SIGHUP, SIG_IGN);
  RegSigHandler(SIGTERM, SIG_IGN);  // kill pid
  RegSigHandler(SIGPIPE, SIG_IGN);
  /* (*) quit, generated from terminal special char */
  RegSigHandler(SIGQUIT, SIG_IGN);  // ctrl+\代表退出SIGQUIT
  /* (*) illegal instruction (not reset when caught)*/
  RegSigHandler(SIGILL, SIG_IGN);  // ctrl+\代表退出SIGQUIT
  /* (*) trace trap (not reset when caught) */
  RegSigHandler(SIGTRAP, SIG_IGN);  // ctrl+\代表退出SIGQUIT
  /* (*) abort process */
  RegSigHandler(SIGABRT, SIG_IGN);  // ctrl+\代表退出SIGQUIT

  /*
   * #define    SIGTERM        15
   * #define    SIGKILL        9
   * kill和kill -9, 两个命令在linux中都有杀死进程的效果
   * 然而两命令的执行过程却大有不同，在程序中如果用错了，可能会造成莫名其妙的现象。
   * 执行kill pid命令，系统会发送一个SIGTERM信号给对应的程序。
   * 执行kill -9 pid命令，系统给对应程序发送的信号是SIGKILL，即exit.
   * exit信号不会被系统阻塞，所以kill -9能顺利杀掉进程 */
  // SIGSTOP和SIGKILL信号是不可捕获的,所以下面两句话写了等于没有写
  RegSigHandler(SIGKILL, SIG_IGN);  // kill -9 pid
  RegSigHandler(SIGSTOP, SIG_IGN);  /* (@) stop (cannot be caught or ignored) */
  RegSigHandler(SIGSTOP, SignalHandler);  // ctrl+z代表停止

  /* (*) bus error (specification exception) */
  RegSigHandler(SIGBUS, SignalHandler);
  RegSigHandler(SIGSEGV, SignalHandler);
  RegSigHandler(SIGILL, SignalHandler);

  RegSigHandler(SIGSYS, SignalHandler); /* (*) bad argument to system call */
  RegSigHandler(SIGPIPE,
                SignalHandler); /* write on a pipe with no one to read it */

  signal(SIGURG, SIG_IGN);  /* (+) urgent contition on I/O channel */
  signal(SIGSTOP, SIG_IGN); /* (@) stop (cannot be caught or ignored) */
  signal(SIGTSTP, SIG_IGN); /* (@) interactive stop */
  signal(SIGCONT, SIG_IGN); /* (!) continue (cannot be caught or ignored) */
  // signal(SIGCHLD ,SIG_IGN);    /* (+) sent to parent on child stop or exit */
  signal(SIGTTIN,
         SIG_IGN); /* (@) background read attempted from control terminal*/
  signal(SIGTTOU,
         SIG_IGN); /* (@) background write attempted to control terminal */
  signal(SIGIO, SIG_IGN);   /* (+) I/O possible, or completed */
  signal(SIGXCPU, SIG_IGN); /* cpu time limit exceeded (see setrlimit()) */
  signal(SIGXFSZ, SIG_IGN); /* file size limit exceeded (see setrlimit()) */

  signal(SIGWINCH, SIG_IGN); /* (+) window size changed */
  signal(SIGPWR, SIG_IGN);   /* (+) power-fail restart */
  signal(SIGUSR1, SIG_IGN);  /* user defined signal 1 */
  signal(SIGUSR2, SIG_IGN);  /* user defined signal 2 */
  signal(SIGPROF, SIG_IGN);  /* profiling time alarm (see setitimer) */
}

void SignalReplace() {
  struct sigaction sa_old;
  struct sigaction sa_new;
  /* do not allow ctrl-c for now... */
  memset(&sa_old, 0, sizeof(struct sigaction));
  memset(&sa_new, 0, sizeof(struct sigaction));
  sa_new.sa_handler = SignalHandler;
  sigemptyset(&sa_new.sa_mask);
  sa_new.sa_flags = 0;
  sigaction(SIGINT, &sa_new, &sa_old);
}

}  // namespace MSF
