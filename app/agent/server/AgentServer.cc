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
#include <proto/Protocol.h>
#include <sock/Socket.h>
#include <sys/epoll.h>

using namespace MSF::AGENT;

namespace MSF {
namespace AGENT {

AgentServer::AgentServer() {
  maxBytes_ = 64 * 1024 * 1024; /* default is 64MB */
  ipAddr4_ = "127.0.0.1";
  tcpPort_ = 8888;
  perConnsAlloc_ = 8;

  if (confFile_.empty()) {
    confFile_ = DEFAULT_CONF_PATH;
  }
  if (logFile_.empty()) {
    logFile_ = DEFAULT_LOG_PATH;
  }
}

AgentServer::~AgentServer() {}

void AgentServer::showUsage() {
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

void AgentServer::parseOption(int argc, char **argv) {
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
        printf("c: %s\n", optarg);
        confFile_ = std::string(optarg);
        break;
      case 's':
        printf("s: %s\n", optarg);
        ipAddr4_ = std::string(optarg);
        break;
      case 'p':
        printf("p: %s\n", optarg);
        tcpPort_ = atoi(optarg);
        break;
      case 'g':
        printf("d: %s\n", optarg);
        break;
      case 'd':
        printf("d: %s\n", optarg);
        daemon_ = static_cast<bool>(atoi(optarg));
        break;
      case 'v':
        printf("v: %s\n", optarg);
        MSF::BuildInfo();
        exit(0);
      case 'h':
        showUsage();
        exit(0);
      case '?':
      default:
        showUsage();
        exit(1);
    }
  }
  return;
}

bool AgentServer::initConfig() {
  IniFile ini;
  assert(ini.Load(confFile_) == 0);

  if (!ini.HasSection("Logger") || !ini.HasSection("System") ||
      !ini.HasSection("Network") || !ini.HasSection("Protocol") ||
      !ini.HasSection("Plugins")) {
    MSF_ERROR << "Confiure invalid, check sections";
    return false;
  }

  assert(ini.GetStringValue("", "Version", &version_) == 0);
  assert(ini.GetIntValue("Logger", "logLevel", &logLevel_) == 0);
  assert(ini.GetStringValue("Logger", "logFile", &logFile_) == 0);

  assert(ini.GetStringValue("System", "pidFile", &pidFile_) == 0);
  assert(ini.GetBoolValue("System", "daemon", &daemon_) == 0);
  assert(ini.GetIntValue("System", "maxThread", &maxThread_) == 0);
  assert(ini.GetIntValue("System", "maxQueue", &maxQueue_) == 0);

  assert(ini.GetValues("Plugins", "plugins", &pluginsList_) == 0);

  assert(ini.GetStringValue("Network", "unixPath", &unixPath_) == 0);
  assert(ini.GetIntValue("Network", "backLog", &backLog_) == 0);
  assert(ini.GetStringValue("Network", "ipAddr4", &ipAddr4_) == 0);
  assert(ini.GetStringValue("Network", "ipAddr6", &ipAddr6_) == 0);
  assert(ini.GetIntValue("Network", "tcpPort", &tcpPort_) == 0);
  assert(ini.GetIntValue("Network", "udpPort", &udpPort_) == 0);
  assert(ini.GetStringValue("Network", "unixPath", &unixPath_) == 0);
  assert(ini.GetStringValue("Network", "unixMask", &unixMask_) == 0);

  assert(ini.GetBoolValue("Protocol", "authChap", &authChap_) == 0);
  assert(ini.GetStringValue("Protocol", "packType", &packType_) == 0);

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

void AgentServer::succConn(AgentConnPtr c) {
  MSF_INFO << "Succ conn for fd: " << c->fd();
}

void AgentServer::handleAgentLogin(AgentConnPtr c, Agent::AgentBhs &bhs,
                                   struct iovec &head) {
  struct iovec body = c->readIovec();
  Agent::LoginRequest login;
  login.ParseFromArray(body.iov_base, proto_->pduLen(bhs));
  mpool_->free(body.iov_base);

  MSF_INFO << "\n Login "
           << "\n name: " << login.name() << "\n cid: " << proto_->srcId(bhs);
  c->setCid(static_cast<uint32_t>(proto_->srcId(bhs)));
  activeConns_[proto_->srcId(bhs)] = c;
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

void AgentServer::handleAgentRequest(AgentConnPtr c, Agent::AgentBhs &bhs,
                                     struct iovec &head) {
  // debugAgentBhs(bhs);
  auto itor = activeConns_.find(proto_->dstId(bhs));
  MSF_INFO << "Peer cid: " << proto_->dstId(bhs);
  if (itor != activeConns_.end()) {
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

bool AgentServer::verifyAgentBhs(const Agent::AgentBhs &bhs) {
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

void AgentServer::handleAgentPdu(AgentConnPtr c, struct iovec &head) {}

void AgentServer::readConn(AgentConnPtr c) {
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

    if (!verifyAgentBhs(bhs)) {
      c->doConnClose();
      return;
    }
    switch (static_cast<Agent::Command>(proto_->command(bhs))) {
      case Agent::Command::CMD_REQ_NODE_REGISTER:
        handleAgentLogin(c, bhs, head);
        break;
      default:
        handleAgentRequest(c, bhs, head);
        break;
    }
  }
}

void AgentServer::writeConn(AgentConnPtr c) {
  if (unlikely(c->fd() <= 0)) {
    return;
  }
  MSF_INFO << "Write conn for fd: " << c->fd();
  c->doConnWrite();
}

void AgentServer::freeConn(AgentConnPtr c) {
  if (unlikely(c->fd() <= 0)) {
    return;
  }
  MSF_INFO << "Close conn for fd: " << c->fd();
  c->doConnClose();
  std::lock_guard<std::mutex> lock(mutex_);
  activeConns_.erase(static_cast<Agent::AppId>(c->cid()));
  freeConns_.push_back(c);
}

/* Callback when acceptor epoll read event hanppen */
void AgentServer::newConn(const int fd, const uint16_t event) {
  std::lock_guard<std::mutex> lock(mutex_);
  /* If single thread to handle accept, so no mutex guard */
  if (unlikely(freeConns_.empty())) {
    for (int i = 0; i < perConnsAlloc_; ++i) {
      AgentConnPtr conn = std::make_shared<AgentConn>();
      if (conn == nullptr) {
        MSF_ERROR << "Fail to alloc connection for fd: " << fd;
        close(fd);
        return;
      }
      freeConns_.push_back(conn);
    }
  }

  AgentConnPtr c = freeConns_.front();
  freeConns_.pop_front();

  MSF_INFO << "Alloc connection for fd: " << fd;

  c->init(mpool_, stack_->getHashLoop(), fd);
  c->setSuccCallback(std::bind(&AgentServer::succConn, this, c));
  c->setReadCallback(std::bind(&AgentServer::readConn, this, c));
  c->setWriteCallback(std::bind(&AgentServer::writeConn, this, c));
  c->setCloseCallback(std::bind(&AgentServer::freeConn, this, c));
  c->enableBaseEvent();
  return;
}

bool AgentServer::initListen() {
  for (uint32_t i = 0; i < 1; ++i) {
    InetAddress addr(ipAddr4_.c_str(), tcpPort_, AF_INET, SOCK_STREAM);
    std::shared_ptr<Acceptor> actor = std::make_shared<Acceptor>(
        stack_->getBaseLoop(), addr,
        std::bind(&AgentServer::newConn, this, std::placeholders::_1,
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

void AgentServer::init(int argc, char **argv) {
  parseOption(argc, argv);

  if (!IsFileExist(logFile_)) {
    std::string logDIR = GetRealPath(logFile_);
    if (!IsDirsExist(logDIR)) {
      MSF_INFO << "Logger dir " << logDIR << " not exist";
      assert(CreateFullDir(logDIR));
    }
  }

  assert(Logger::getLogger().init(logFile_.c_str()));

  assert(initConfig());

  mpool_ = new MemPool();
  assert(mpool_);
  assert(mpool_->init());

  proto_ = std::make_unique<AgentProto>();
  assert(proto_);

  stack_ = new EventStack();
  assert(stack_);

  std::vector<struct ThreadArg> threadArgs;
  threadArgs.push_back(std::move(ThreadArg("AgentLoop")));
  assert(stack_->startThreads(threadArgs));

  assert(initListen());
}

void AgentServer::start() {
  stack_->start();
  return;
}

}  // namespace AGENT
}  // namespace MSF

int main(int argc, char *argv[]) {
  AgentServer srv = AgentServer();
  srv.init(argc, argv);
  srv.start();
  return 0;
}
