/**************************************************************************
 *
 * Copyright (c) 2017-2021, luotang.me <wypx520@gmail.com>, China.
 * All rights reserved.
 *
 * Distributed under the terms of the GNU General Public License v2.
 *
 * This software is provided 'as is' with no explicit or implied warranties
 * in respect of its properties, including, but not limited to, correctness
 * and/or fitness for purposc->
 *
 **************************************************************************/
#include "AgentServer.h"

#include <assert.h>
#include <base/File.h>
#include <base/IniFile.h>
#include <base/Logger.h>
#include <base/Utils.h>
#include <base/Version.h>
#include <getopt.h>
#include <sock/Socket.h>
#include <sys/epoll.h>

using namespace MSF::AGENT;

namespace MSF {
namespace AGENT {

AgentServer::AgentServer() {
  max_bytes_ = 64 * 1024 * 1024; /* default is 64MB */
  ip_addr4_ = "127.0.0.1";
  tcp_port_ = 8888;
  per_alloc_conns_ = 8;
}

AgentServer::~AgentServer() {}

void AgentServer::DebugInfo() {
  std::cout << std::endl;
  std::cout << "Agent Server Options:" << std::endl;
  std::cout << " -c, --conf=FILE        The configure file." << std::endl;
  std::cout << " -s, --host=HOST        The host that server will listen on"
            << std::endl;
  std::cout << " -p, --port=PORT        The port that server will listen on"
            << std::endl;
  std::cout << " -d, --daemon           Run as daemon." << std::endl;
  std::cout << " -g, --signal           restart|kill|reload" << std::endl;
  std::cout << "                        restart: restart process graceful"
            << std::endl;
  std::cout << "                        kill: fast shutdown graceful"
            << std::endl;
  std::cout << "                        reload: parse config file and reinit"
            << std::endl;
  std::cout << " -v, --version          Print the version." << std::endl;
  std::cout << " -h, --help             Print help info." << std::endl;
  std::cout << std::endl;

  std::cout << "Examples:" << std::endl;
  std::cout
      << " ./AgentServer -c AgentServer.conf -s 127.0.0.1 -p 8888 -d -g kill"
      << std::endl;

  std::cout << std::endl;
  std::cout << "Reports bugs to <luotang.me>" << std::endl;
  std::cout << "Commit issues to <https://github.com/wypx/libmsf>" << std::endl;
  std::cout << std::endl;
}

void AgentServer::ParseOption(int argc, char **argv) {
  /* 浅谈linux的命令行解析参数之getopt_long函数
   * https://blog.csdn.net/qq_33850438/article/details/80172275
   **/
  int c;
  while (true) {
    int optIndex = 0;
    static struct option longOpts[] = {
        {"conf", required_argument, nullptr, 'c'},
        {"host", required_argument, nullptr, 's'},
        {"port", required_argument, nullptr, 'p'},
        {"signal", required_argument, nullptr, 'g'},
        {"daemon", no_argument, nullptr, 'd'},
        {"version", no_argument, nullptr, 'v'},
        {"help", no_argument, nullptr, 'h'},
        {0, 0, 0, 0}};
    c = getopt_long(argc, argv, "c:s:p:g:dvh?", longOpts, &optIndex);
    if (c == -1) {
      break;
    }
    switch (c) {
      case 'c':
        config_file_ = std::string(optarg);
        break;
      case 's':
        ip_addr4_ = std::string(optarg);
        break;
      case 'p':
        tcp_port_ = atoi(optarg);
        break;
      case 'g':
        break;
      case 'd':
        daemon_ = true;
        break;
      case 'v':
        MSF::BuildInfo();
        exit(0);
      case 'h':
        DebugInfo();
        exit(0);
      case '?':
      default:
        DebugInfo();
        exit(1);
    }
  }
  return;
}

bool AgentServer::LoadConfig() {
  IniFile ini;
  assert(ini.Load(config_file_) == 0);

  if (!ini.HasSection("Logger") || !ini.HasSection("System") ||
      !ini.HasSection("Network") || !ini.HasSection("Protocol") ||
      !ini.HasSection("Plugins")) {
    MSF_ERROR << "Confiure invalid, check sections";
    return false;
  }
#if 0
  assert(ini.GetStringValue("", "Version", &version_) == 0);
  assert(ini.GetIntValue("Logger", "logLevel", &log_level_) == 0);
  assert(ini.GetStringValue("Logger", "logFile", &log_dir_) == 0);

  assert(ini.GetStringValue("System", "pidFile", &pid_file_) == 0);
  assert(ini.GetBoolValue("System", "daemon", &daemon_) == 0);
  assert(ini.GetIntValue("System", "maxThread", &work_thread_num_) == 0);

  assert(ini.GetValues("Plugins", "plugins", &plugins_) == 0);

  assert(ini.GetIntValue("Network", "backLog", &back_log_) == 0);
  assert(ini.GetStringValue("Network", "ipAddr4", &ip_addr4_) == 0);
  assert(ini.GetStringValue("Network", "ipAddr6", &ip_addr6_) == 0);
  assert(ini.GetIntValue("Network", "tcpPort", &tcp_port_) == 0);
  assert(ini.GetIntValue("Network", "udpPort", &udp_port_) == 0);
  assert(ini.GetStringValue("Network", "unixPath", &unix_path_) == 0);
  assert(ini.GetStringValue("Network", "unixMask", &unix_mask_) == 0);

  assert(ini.GetBoolValue("Protocol", "authChap", &agent_auth_chap_) == 0);
  assert(ini.GetStringValue("Protocol", "packType", &agent_pack_type_) == 0);
#endif
  // ini.setValue("COMMON", "DB", "sys", "数据库");
  // ini.setValue("COMMON", "PASSWD", "root", "数据库密码");
  // ini.setValue("", "NAME", "cxy", "");

  // ini.SaveAs("srv_agent.conf");

  // ini.DeleteSection("Test");
  // ini.SaveAs("srv_agent.conf");

  // ini.DeleteKey("Test2", "test_key2");
  // ini.SaveAs("srv_agent.conf");
  return true;
}

void AgentServer::SuccConn(AgentConnPtr c) {
  MSF_INFO << "Succ conn for fd: " << c->fd();
}

void AgentServer::HandleAgentLogin(AgentConnPtr c, Agent::AgentBhs &bhs,
                                   struct iovec &head) {
  struct iovec body = c->readIovec();
  Agent::LoginRequest login;
  login.ParseFromArray(body.iov_base, proto_->pduLen(bhs));
  mpool_->Free(body.iov_base);

  MSF_INFO << "\n Login "
           << "\n name: " << login.name() << "\n cid: " << proto_->srcId(bhs);
  c->setCid(static_cast<uint32_t>(proto_->srcId(bhs)));
  active_conns_[proto_->srcId(bhs)] = c;
  {
    Agent::AppId tmpId = proto_->srcId(bhs);
    proto_->setSrcId(bhs, proto_->dstId(bhs));
    proto_->setDstId(bhs, tmpId);
  }
  proto_->setOpcode(bhs, Agent::Opcode::OP_RSP);
  proto_->setRetCode(bhs, Agent::Errno::ERR_EXEC_SUCESS);
  proto_->setPduLen(bhs, 0);
  bhs.SerializeToArray(head.iov_base, AGENT_HEAD_LEN);

  proto_->debugBhs(bhs);

  c->writeIovec(head);
  c->enableWriting();
}

void AgentServer::HandleAgentRequest(AgentConnPtr c, Agent::AgentBhs &bhs,
                                     struct iovec &head) {
  // debugAgentBhs(bhs);
  auto itor = active_conns_.find(proto_->dstId(bhs));
  MSF_INFO << "Peer cid: " << proto_->dstId(bhs);
  if (itor != active_conns_.end()) {
    AgentConnPtr peer = (AgentConnPtr)itor->second;
    MSF_INFO << "\n Send cmd to"
             << "\n cmd: " << proto_->command(bhs) << "\n cid: " << peer->cid();
    assert(proto_->dstId(bhs) == static_cast<Agent::AppId>(peer->cid()));
    // http://blog.chinaunix.net/uid-14949191-id-3967282.html
    // std::swap(bhs->srcId_, bhs->dstId_);
    // {
    //   AgentAppId tmpId = bhs->srcId_;
    //   bhs->srcId_ = bhs->dstId_;
    //   bhs->dstId_ = tmpId;
    // }
    peer->writeIovec(head);
    if (proto_->pduLen(bhs) > 0) {
      peer->writeIovec(c->readIovec());
    }
    peer->enableWriting();
    peer->wakeup();
  } else {
    MSF_INFO << "\n Peer offline"
             << "\n cmd: " << proto_->command(bhs);
    {
      Agent::AppId tmpId = proto_->srcId(bhs);
      proto_->setSrcId(bhs, proto_->dstId(bhs));
      proto_->setDstId(bhs, tmpId);
    }
    proto_->setOpcode(bhs, Agent::Opcode::OP_RSP);
    proto_->setRetCode(bhs, Agent::Errno::ERR_PEER_OFFLINE);
    proto_->setPduLen(bhs, 0);

    bhs.SerializeToArray(head.iov_base, AGENT_HEAD_LEN);
    c->writeIovec(head);
    c->enableWriting();
    c->wakeup();
  }
}

bool AgentServer::VerifyAgentBhs(const Agent::AgentBhs &bhs) {
  if (unlikely(proto_->magic(bhs) != kAgentMagic)) {
    MSF_ERROR << "Unkown agent magic from: " << proto_->srcId(bhs);
    return false;
  }
  if (unlikely(proto_->version(bhs) != kAgentVersion)) {
    MSF_ERROR << "Unkown agent version from: " << proto_->srcId(bhs);
    return false;
  }
  return true;
}

void AgentServer::HandleAgentPdu(AgentConnPtr c, struct iovec &head) {}

void AgentServer::ReadConn(AgentConnPtr c) {
  if (unlikely(c->fd() <= 0)) {
    return;
  }

  MSF_INFO << "Read conn for fd: " << c->fd();

  c->doConnRead();

  while (c->iovCount() > 0) {
    MSF_INFO << "======> handle iovec: " << c->iovCount();
    struct iovec head = c->readIovec();
    Agent::AgentBhs bhs;
    bhs.ParseFromArray(head.iov_base, AGENT_HEAD_LEN);
    proto_->debugBhs(bhs);

    if (!VerifyAgentBhs(bhs)) {
      c->doConnClose();
      return;
    }
    switch (static_cast<Agent::Command>(proto_->command(bhs))) {
      case Agent::Command::CMD_REQ_NODE_REGISTER:
        HandleAgentLogin(c, bhs, head);
        break;
      default:
        HandleAgentRequest(c, bhs, head);
        break;
    }
  }
}

void AgentServer::WriteConn(AgentConnPtr c) {
  if (unlikely(c->fd() <= 0)) {
    return;
  }
  MSF_INFO << "Write conn for fd: " << c->fd();
  c->doConnWrite();
}

void AgentServer::FreeConn(AgentConnPtr c) {
  if (unlikely(c->fd() <= 0)) {
    return;
  }
  MSF_INFO << "Close conn for fd: " << c->fd();
  c->doConnClose();
  std::lock_guard<std::mutex> lock(mutex_);
  active_conns_.erase(static_cast<Agent::AppId>(c->cid()));
  free_conns_.push_back(c);
}

/* Callback when acceptor epoll read event hanppen */
void AgentServer::NewConn(const int fd, const uint16_t event) {
  std::lock_guard<std::mutex> lock(mutex_);
  /* If single thread to handle accept, so no mutex guard */
  if (unlikely(free_conns_.empty())) {
    for (uint32_t i = 0; i < per_alloc_conns_; ++i) {
      AgentConnPtr conn = std::make_shared<AgentConn>();
      if (conn == nullptr) {
        MSF_ERROR << "Fail to alloc connection for fd: " << fd;
        close(fd);
        return;
      }
      free_conns_.push_back(conn);
    }
  }

  AgentConnPtr c = free_conns_.front();
  free_conns_.pop_front();

  MSF_INFO << "Alloc connection for fd: " << fd;

  c->init(mpool_, stack_->getHashLoop(), fd);
  c->setSuccCallback(std::bind(&AgentServer::SuccConn, this, c));
  c->setReadCallback(std::bind(&AgentServer::ReadConn, this, c));
  c->setWriteCallback(std::bind(&AgentServer::WriteConn, this, c));
  c->setCloseCallback(std::bind(&AgentServer::FreeConn, this, c));
  c->enableBaseEvent();
  return;
}

bool AgentServer::InitNetwork() {
  for (uint32_t i = 0; i < 1; ++i) {
    InetAddress addr(ip_addr4_.c_str(), tcp_port_, AF_INET, SOCK_STREAM);
    std::shared_ptr<Acceptor> actor = std::make_shared<Acceptor>(
        stack_->getBaseLoop(), addr,
        std::bind(&AgentServer::NewConn, this, std::placeholders::_1,
                  std::placeholders::_2));
    if (!actor) {
      MSF_INFO << "Fail to alloc acceptor for " << addr.hostPort2String();
      return false;
    }
    Event *e = new Event(stack_->getBaseLoop(), actor->sfd());
    if (!e) {
      MSF_INFO << "Fail to alloc event for " << addr.hostPort2String();
      return false;
    }
    e->setReadCallback(std::bind(&Acceptor::acceptCb, actor));
    e->setCloseCallback(std::bind(&Acceptor::errorCb, actor));
    e->disableWriting();
    e->enableClosing();
    e->enableReading();
    actors_.push_back(actor);
  }

  return true;
}

void AgentServer::Init(int argc, char **argv) {
  ParseOption(argc, argv);

  // if (!IsFileExist(log_dir_)) {
  //   std::string logDIR = GetRealPath(log_dir_);
  //   if (!IsDirsExist(logDIR)) {
  //     MSF_INFO << "Logger dir " << logDIR << " not exist";
  //     assert(CreateFullDir(logDIR));
  //   }
  // }

  assert(Logger::getLogger().init(log_dir_.c_str()));

  assert(LoadConfig());

  mpool_ = new MemPool();
  assert(mpool_);
  assert(mpool_->Init());

  proto_ = std::make_unique<AgentProto>();
  assert(proto_);

  stack_ = new EventStack();
  assert(stack_);

  std::vector<struct ThreadArg> threadArgs;
  threadArgs.push_back(std::move(ThreadArg("AgentLoop")));
  assert(stack_->startThreads(threadArgs));

  assert(InitNetwork());
}

void AgentServer::Start() {
  stack_->start();
  return;
}

}  // namespace AGENT
}  // namespace MSF

int main(int argc, char *argv[]) {
  AgentServer srv = AgentServer();
  srv.Init(argc, argv);
  srv.Start();
  return 0;
}
