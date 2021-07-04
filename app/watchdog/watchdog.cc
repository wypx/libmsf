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
#include "watchdog.h"

#include <base/process.h>
#include <base/signal_manager.h>
#include <base/version.h>
#include <base/mem_pool.h>
#include <base/logging.h>

#include "echo_server.h"

#include <cassert>

#if 0
#include <gflags/gflags.h>

#include <base/logging.h>
#include <butil/time.h>
#include <brpc/channel.h>
#include "../mobile/src/Mobile.pb.h"

DEFINE_bool(send_attachment, true, "Carry attachment along with requests");
DEFINE_string(protocol, "baidu_std",
              "Protocol type. Defined in src/brpc/options.proto");
DEFINE_string(connection_type, "",
              "Connection type. Available values: single, pooled, short");
DEFINE_string(server, "0.0.0.0:8001", "IP Address of server");
DEFINE_string(load_balancer, "", "The algorithm for load balancing");
DEFINE_int32(timeout_ms, 100, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)");
#endif

using namespace MSF;

namespace MSF {

#if 0
void HandleGetMobileAPNResponse(brpc::Controller *cntl,
                                Mobile::GetMobileAPNResponse *response) {
  // std::unique_ptr makes sure cntl/response will be deleted before returning.
  std::unique_ptr<brpc::Controller> cntl_guard(cntl);
  std::unique_ptr<Mobile::GetMobileAPNResponse> response_guard(response);

  if (cntl->Failed()) {
    LOG(WARNING) << "Fail to send EchoRequest, " << cntl->ErrorText();
    return;
  }
  LOG(INFO) << "Received response from " << cntl->remote_side() << ": "
            << response->apn() << " (attached=" << cntl->response_attachment()
            << ")"
            << " latency=" << cntl->latency_us() << "us";
}
#endif

Guard::Guard() {
  // logFile_ = "/var/log/luotang.me/Guard.log";
  // assert(Logger::getLogger().init(logFile_.c_str()));

  stack_ = new EventStack();
  assert(stack_);

  std::vector<struct ThreadArg> threadArgs;
  threadArgs.push_back(std::move(ThreadArg("AgentLoop")));
  assert(stack_->StartThreads(threadArgs));

  // agent_ = new AgentClient(stack_->getOneLoop(), "Guard", Agent::APP_GUARD,
  // "luotang.me", 8888);
  // agent_ = new AgentClient(stack_->getOneLoop(), "Guard", Agent::APP_GUARD);
  // assert(agent_);
}

Guard::~Guard() {}

void Guard::start() { stack_->Start(); }

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
  // struct ApnItem item = {0};
  // AgentPduPtr pdu = std::make_shared<AgentPdu>();
  // pdu->addRspload(&item, sizeof(item));
  // pdu->cmd_ = Agent::Command::CMD_REQ_MOBILE_READ;
  // pdu->dstId_ = Agent::AppId::APP_MOBILE;
  // pdu->timeOut_ = 5;
  // while (1) {
  //   int ret = agent_->sendPdu(pdu);
  //   if (Agent::ERR_AGENT_NOT_START != ret) {
  //     break;
  //   }
  // }
  // LOG(INFO) << "ApnItem cid: " << item.cid << " active: " << item.active;
}

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
  LOG(INFO) << "optind: " << optind << ", opterr: " << opterr;

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

#include <libgen.h>

std::string GetFileName(const std::string &path) {
  char ch = '/';
#ifdef _WIN32_
  ch = '\\';
#endif
  size_t pos = path.rfind(ch);
  if (pos == std::string::npos)
    return path;
  else
    return path.substr(pos + 1);
}

std::string GetDirName(const std::string &path) {
  char ch = '/';
#ifdef _WIN32_
  ch = '\\';
#endif
  size_t pos = path.rfind(ch);
  if (pos == std::string::npos)
    return path;
  else
    return path.substr(0, pos + 1);
}

std::string GetParentDirName(const std::string &path) {
  std::string parent = path;
  if (parent == "/") {
    return parent;
  }
  if (parent[parent.size() - 1] == '/') {
    parent.erase(parent.size() - 1);
  }
  return GetDirName(parent);
}

int main(int argc, char *argv[]) {
  BuildInfo();

  EventLoop loop;

  InetAddress addr("127.0.0.1", 8888, AF_INET, SOCK_STREAM);
  FastRpcServer *server = new FastRpcServer(&loop, addr);

  EchoServiceImpl echo_impl(1);
  server->HookService(&echo_impl);

  loop.EnterLoop();

#if 0
  logging::LoggingSettings log_setting;
  log_setting.logging_dest = logging::LOG_TO_ALL;
  log_setting.log_file = "/home/luotang.me/log/Guard.log";
  log_setting.delete_old = logging::APPEND_TO_OLD_LOG_FILE;
  logging::InitLogging(log_setting);
  // Parse gflags. We recommend you to use gflags as well.
  google::ParseCommandLineFlags(&argc, &argv, true);

  // A Channel represents a communication line to a Server. Notice that
  // Channel is thread-safe and can be shared by all threads in your program.
  brpc::Channel channel;

  // Initialize the channel, NULL means using default options.
  brpc::ChannelOptions options;
  options.protocol = FLAGS_protocol;
  options.connection_type = FLAGS_connection_type;
  options.timeout_ms = FLAGS_timeout_ms /*milliseconds*/;
  options.max_retry = FLAGS_max_retry;
  if (channel.Init(FLAGS_server.c_str(), FLAGS_load_balancer.c_str(),
                   &options) != 0) {
    LOG(ERROR) << "Fail to initialize channel";
    return -1;
  }

  // Normally, you should not call a Channel directly, but instead construct
  // a stub Service wrapping it. stub can be shared by all threads as well.
  Mobile::GetMobileAPNService_Stub stub(&channel);

  // Send a request and wait for the response every 1 second.
  int log_id = 0;
  while (!brpc::IsAskedToQuit()) {
    // Since we are sending asynchronous RPC (`done' is not NULL),
    // these objects MUST remain valid until `done' is called.
    // As a result, we allocate these objects on heap
    Mobile::GetMobileAPNResponse *response = new Mobile::GetMobileAPNResponse();
    brpc::Controller *cntl = new brpc::Controller();

    // Notice that you don't have to new request, which can be modified
    // or destroyed just after stub.Echo is called.
    Mobile::GetMobileAPNRequest request;
    request.set_cid(100);

    cntl->set_log_id(log_id++);  // set by user
    if (FLAGS_send_attachment) {
      // Set attachment which is wired to network directly instead of
      // being serialized into protobuf messages.
      cntl->request_attachment().append("foo");
    }

    // We use protobuf utility `NewCallback' to create a closure object
    // that will call our callback `HandleEchoResponse'. This closure
    // will automatically delete itself after being called once
    google::protobuf::Closure *done =
        brpc::NewCallback(&HandleGetMobileAPNResponse, cntl, response);
    stub.GetMobileAPN(cntl, &request, response, done);

    // This is an asynchronous RPC, so we can only fetch the result
    // inside the callback
    sleep(1);
  }
#endif

  LOG(INFO) << "EchoClient is going to quit";

  OptionParser(argc, argv);

  std::string dir_1 = "/";
  std::string dir_2 = "/test/";
  std::string dir_3 = "/test/aaaa/";

  LOG(INFO) << "dir_1: " << GetParentDirName(dir_1);
  LOG(INFO) << "dir_2: " << GetParentDirName(dir_2);
  LOG(INFO) << "dir_3: " << GetParentDirName(dir_3);

  Guard guard = Guard();

  // SignalReplace();
  /*设置创建文件的mask值，这里只有运行用户的rw权限生效*/
  umask(0177);

  guard.sendPdu();

  guard.start();

  return 0;
}