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
#include "Guard.h"

#include <base/Logger.h>
#include <base/mem/MemPool.h>
#include <base/Process.h>
#include <base/Signal.h>
#include <base/Version.h>

#include <cassert>

using namespace MSF;
using namespace MSF::AGENT;
using namespace MSF::GUARD;

namespace MSF {
namespace GUARD {

Guard::Guard() {
  logFile_ = "/var/log/luotang.me/Guard.log";
  assert(Logger::getLogger().init(logFile_.c_str()));

  stack_ = new EventStack();
  assert(stack_);

  std::vector<struct ThreadArg> threadArgs;
  threadArgs.push_back(std::move(ThreadArg("AgentLoop")));
  assert(stack_->startThreads(threadArgs));

  // agent_ = new AgentClient(stack_->getOneLoop(), "Guard", Agent::APP_GUARD, "luotang.me", 8888);
  agent_ = new AgentClient(stack_->getOneLoop(), "Guard", Agent::APP_GUARD);
  assert(agent_);
}

Guard::~Guard() {}

void Guard::start() { stack_->start(); }

struct ApnItem {
  int cid;    /* Context ID, uniquely identifies this call */
  int active; /* 0=inactive, 1=active/physical link down, 2=active/physical link
                 up */
  char *type; /* One of the PDP_type values in TS 27.007 section 10.1.1.
                 For example, "IP", "IPV6", "IPV4V6", or "PPP". */
  char *ifname;  /* The network interface name */
  char *apn;     /* ignored */
  char *address; /* An address, e.g., "192.0.1.3" or "2001:db8::1". */
};

void Guard::sendPdu() {
  struct ApnItem item = {0};
  AgentPduPtr pdu = std::make_shared<AgentPdu>();
  pdu->addRspload(&item, sizeof(item));
  pdu->cmd_ = Agent::Command::CMD_REQ_MOBILE_READ;
  pdu->dstId_ = Agent::AppId::APP_MOBILE;
  pdu->timeOut_ = 5;
  while (1) {
    int ret = agent_->sendPdu(pdu);
    if (Agent::ERR_AGENT_NOT_START != ret) {
      break;
    }
  }
  MSF_INFO << "ApnItem cid: " << item.cid << " active: " << item.active;
}

}  // namespace GUARD
}  // namespace MSF

/**
 * 1是用来保存选项的参数的
 * 2用来记录下一个检索位置;
 * 3表示的是是否将错误信息输出到stderr,为0时表示不输出
 * 4表示不在选项字符串optstring中的选项
 * hvdc:k:? 表示什么呢?
 * 这就是一个选项字符串,对应到命令行就是-h,-v,-d,-c,-k .
 * 冒号又是什么呢?
 * 冒号表示参数,一个冒号就表示这个选项后面必须带有参数(可以连着或者空格隔开)
 * 两个冒号的就表示这个选项的参数是可选的,即可以有参数,也可以没有参数
 * 但要注意有参数时,参数与选项之间不能有空格,这一点和一个冒号时是有区别的*/
extern char *optarg;
extern int optind;
extern int opterr;
extern int optopt;

#define DEF_MSF_DAEMON_NAME "Guard"
#define DEF_MSF_DAEMON_CONFIG_PATH "/home/msf_guard_conf.json"

#define DEF_MSF_KEY_AUTHOR "msf_author"
#define DEF_MSF_KEY_VERSION "msf_version"
#define DEF_MSF_KEY_DESC "msf_desc"
#define DEF_MSF_KEY_PROCS "msf_procs"

#define DEF_PROC_KEY_IDX "proc_idx"
#define DEF_PROC_KEY_NAME "proc_name"
#define DEF_PROC_KEY_PATH "proc_path"
#define DEF_PROC_KEY_CONF "proc_conf"

static struct msf_daemon gmsf;
struct msf_daemon *g_msf = &gmsf;

void ShowUsage(void) {
  fprintf(stderr,
          "Usage: %s [-?hvdc] [-d level] [-c config-file] [-k signal]\n"
          "       -h        Print help message.\n"
          "       -v        Show Version and exit.\n"
          "       -d        daemon mode.\n"
          "       -c file   Use given config-file instead of\n"
          "                 %s\n"
          "       -k reload|rotate|kill|parse\n"
          "                 kill is fast shutdown\n"
          "                 Parse configuration file, then send signal to \n"
          "                 running copy (except -k parse) and exit.\n",
          DEF_MSF_DAEMON_NAME, g_msf->msf_conf);
  exit(1);
}

void ShowVersion(void) {
  uint32_t proc_idx;
  struct process_desc *proc_desc = NULL;

  printf("### Micro service framework debug info ###\n\n");

  printf("### Msf author = (%s)\n", g_msf->msf_author);
  printf("### Msf version = (%s)\n", g_msf->msf_version);
  printf("### Msf desc = (%s)\n\n", g_msf->msf_desc);

  printf("### Msf proc_num = (%d)\n\n", g_msf->msf_proc_num);

  for (proc_idx = 0; proc_idx < g_msf->msf_proc_num; proc_idx++) {
    proc_desc = &(g_msf->msf_proc_desc[proc_idx]);
    printf("### Msf proc_idx = (%d)\n", proc_desc->proc_idx);
    printf("### Msf proc_name = (%s)\n", proc_desc->proc_name);
    printf("### Msf proc_path = (%s)\n", proc_desc->proc_path);
    printf("### Msf proc_conf = (%s)\n\n", proc_desc->proc_conf);
  }

  printf("### Micro service framework debug info ###\n");
}

int OptionParser(int argc, char *argv[]) {
  int c;
  MSF_INFO << "optind: " << optind << ", opterr: " << opterr;

  // getopt_long
  while ((c = getopt(argc, argv, "hvdc:k:?")) != -1) {
    switch (c) {
      case 'h':
        ShowUsage();
        break;
      case 'v':
        ShowVersion();
        break;
      case 'd':
        break;
      case 'c':
        g_msf->msf_conf = strdup(optarg);
        break;
      case 'k':
        break;
      case '?':
      default:
        printf("Unknown option: %c\n", (char)optopt);
        ShowVersion();
        break;
    }
  }
  return 0;
}

int main(int argc, char *argv[]) {
  BuildInfo();

  OptionParser(argc, argv);

  Guard guard = Guard();

  // SignalReplace();
  /*设置创建文件的mask值，这里只有运行用户的rw权限生效*/
  umask(0177);

  guard.sendPdu();

  guard.start();

  return 0;
}