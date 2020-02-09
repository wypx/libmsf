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
#include <base/Mem.h>
#include <base/Utils.h>
#include <proto/Protocol.h>
#include <sock/Socket.h>
#include <sys/epoll.h>

using namespace MSF::AGENT;

namespace MSF {
namespace AGENT {

static const struct {
  enum CONFIG_IDX id;
  const std::string name;
} optionids[] = {
    {CONFIG_LOGLEVEL, "logLevel"}, {CONFIG_LOGFILE, "logFile"},
    {CONFIG_PIDFILE, "pidFile"},   {CONFIG_DAEMON, "daemon"},
    {CONFIG_PLUGINS, "plugins"},   {CONFIG_BACKLOG, "backLog"},
    {CONFIG_MAXCONNS, "maxConns"}, {CONFIG_MAXTHREAD, "maxThread"},
    {CONFIG_MAXQUEUE, "maxQueue"}, {CONFIG_IPADDRV4, "ipAddr4"},
    {CONFIG_IPADDRV6, "ipAddr6"},  {CONFIG_TCPPORT, "tcpPort"},
    {CONFIG_UDPPORT, "udpPort"},   {CONFIG_UNXIPATH, "unixPath"},
    {CONFIG_UNXIMASK, "unixMask"}, {CONFIG_AUTHCHAP, "authChap"},
    {CONFIG_PACKTYPE, "packType"},
};

static inline void debugAgentBhs(struct AgentBhs *bhs) {
  MSF_DEBUG << "###################################";
  MSF_DEBUG << "bhs:";
  MSF_DEBUG << "bhs version: " << bhs->version_;
  MSF_DEBUG << "bhs magic:  " << bhs->magic_;
  MSF_DEBUG << "bhs srcid: " << bhs->srcId_;
  MSF_DEBUG << "bhs dstid: " << bhs->dstId_;
  MSF_DEBUG << "bhs cmd: " << bhs->cmd_;
  MSF_DEBUG << "bhs seq: " << bhs->sessNo_;
  MSF_DEBUG << "bhs errcode: " << bhs->retCode_;
  MSF_DEBUG << "bhs datalen: " << bhs->dataLen_;
  MSF_DEBUG << "bhs restlen: " << bhs->restLen_;
  MSF_DEBUG << "bhs checksum: " << bhs->checkSum_;
  MSF_DEBUG << "bhs timeout: " << bhs->timeOut_;
  MSF_DEBUG << "###################################";
}

AgentServer::AgentServer() {
  maxBytes_ = 64 * 1024 * 1024; /* default is 64MB */
  connPerAlloc_ = 8;
  srvConf_.ipAddr4_ = "127.0.0.1";
  srvConf_.tcpPort_ = 8888;

  if (confFile_.empty()) {
    confFile_ = "//home/luotang.me/conf/AgentServer.conf";
  }
  if (logFile_.empty()) {
    logFile_ = "/home/luotang.me/log/AgentServer.log";
  }
}

AgentServer::~AgentServer() {}

bool AgentServer::initConfig() {
  IniFile ini;
  if (ini.Load(confFile_) != 0) {
    return false;
  }

  if (!ini.HasSection("Logger") || !ini.HasSection("System") ||
      !ini.HasSection("Network") || !ini.HasSection("Protocol") ||
      !ini.HasSection("Plugins")) {
    return false;
  }

  if (ini.GetStringValue("", "Version", &srvConf_.version_) == 0) {
    MSF_INFO << "Version ==> " << srvConf_.version_;
  }

  if (ini.GetIntValue("Logger", "logLevel", &srvConf_.logLevel_) == 0) {
    MSF_INFO << "logLevel ==> " << srvConf_.logLevel_;
  }
  if (ini.GetStringValue("Logger", "logFile", &srvConf_.logFile_) == 0) {
    MSF_INFO << "logFile ==> " << srvConf_.logFile_;
  }

  ini.GetStringValue("System", "pidFile", &srvConf_.pidFile_);
  MSF_INFO << "pidFile ==> " << srvConf_.pidFile_;

  ini.GetBoolValue("System", "daemon", &srvConf_.daemon_);
  MSF_INFO << "daemon ==> " << srvConf_.daemon_;

  ini.GetBoolValue("Protocol", "authChap", &srvConf_.authChap_);
  MSF_INFO << "authChap ==> " << srvConf_.authChap_;

  ini.GetIntValue("System", "maxQueue", &srvConf_.maxQueue_);
  MSF_INFO << "maxQueue ==> " << srvConf_.maxQueue_;
  ini.GetIntValue("System", "maxThread", &srvConf_.maxThread_);
  MSF_INFO << "maxThread ==> " << srvConf_.maxThread_;

  ini.GetValues("Plugins", "plugins", &srvConf_.pluginsList_);
  MSF_INFO << "valueList.size() ==> " << srvConf_.pluginsList_.size();
  MSF_INFO << "valueList(0) ==> " << srvConf_.pluginsList_[0];
  MSF_INFO << "valueList(1) ==> " << srvConf_.pluginsList_[1];

  ini.GetStringValue("Network", "unixPath", &srvConf_.unixPath_);
  MSF_INFO << "unixPath ==> " << srvConf_.unixPath_;
  ini.GetIntValue("Network", "backLog", &srvConf_.backLog_);
  MSF_INFO << "backLog ==> " << srvConf_.backLog_;
  ini.GetStringValue("Network", "ipAddr4", &srvConf_.ipAddr4_);
  MSF_INFO << "ipAddr4 ==> " << srvConf_.ipAddr4_;
  ini.GetStringValue("Network", "ipAddr6", &srvConf_.ipAddr6_);
  MSF_INFO << "ipAddr6 ==> " << srvConf_.ipAddr6_;
  ini.GetIntValue("Network", "tcpPort", &srvConf_.tcpPort_);
  MSF_INFO << "tcpPort ==> " << srvConf_.tcpPort_;
  ini.GetIntValue("Network", "udpPort", &srvConf_.udpPort_);
  MSF_INFO << "udpPort ==> " << srvConf_.udpPort_;
  ini.GetStringValue("Network", "unixPath", &srvConf_.unixPath_);
  MSF_INFO << "unixPath ==> " << srvConf_.unixPath_;
  ini.GetStringValue("Network", "unixMask", &srvConf_.unixMask_);
  MSF_INFO << "unixMask ==> " << srvConf_.unixMask_;

  ini.GetBoolValue("Protocol", "authChap", &srvConf_.authChap_);
  MSF_INFO << "authChap ==> " << srvConf_.authChap_;
  ini.GetStringValue("Protocol", "packType", &srvConf_.packType_);
  MSF_INFO << "packType ==> " << srvConf_.packType_;
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

void AgentServer::succConn(ConnectionPtr c) {
  MSF_INFO << "Succ conn for fd: " << c->fd();
}

bool AgentServer::handleRxIORet(ConnectionPtr c, const int ret) {
  if (unlikely(ret == 0)) {
    c->close();
    return false;
  } else if (ret < 0 && errno != EWOULDBLOCK && errno != EAGAIN &&
             errno != EINTR) {
    c->close();
    return false;
  } else if (ret > 0) {
    c->rxRecved_ += ret;
    if (likely(c->rxRecved_ == c->rxWanted_)) {
      c->rxRecved_ = 0;
      c->rxWanted_ = 0;
      return true;
    }
    c->rxIov_[0].iov_base =
        static_cast<char *>(c->rxIov_[0].iov_base) + c->rxRecved_;
    c->rxIov_[0].iov_len = c->rxWanted_ - c->rxRecved_;
    return false;
  } else {
    // EAGAIN
    return false;
  }
}

bool AgentServer::handleTxIORet(ConnectionPtr c, const int ret) {
  if (unlikely(ret == 0)) {
    c->close();
    return false;
  } else if (ret < 0 && errno != EWOULDBLOCK && errno != EAGAIN &&
             errno != EINTR) {
    c->close();
    return false;
  } else if (ret > 0) {
    c->txSended_ += ret;
    if (likely(c->txSended_ == c->txWanted_)) {
      c->txSended_ = 0;
      c->txWanted_ = 0;
      return true;
    }
    return false;
  } else {
    // EAGAIN
    return false;
  }
}

void AgentServer::handleAgentLogin(ConnectionPtr c) {
  AgentBhs *bhs = static_cast<AgentBhs *>(c->rxIov_[0].iov_base);

  AgentLogin login = {0};
  struct iovec iov;
  iov.iov_base = &login;
  iov.iov_len = sizeof(AgentLogin);
  RecvMsg(c->fd(), &iov, 1, MSG_NOSIGNAL | MSG_WAITALL);
  MSF_INFO << "\n Login "
            << "\n name: " << login.name_ << "\n cid: " << bhs->srcId_;
  c->cid_ = bhs->srcId_;
  activeConns_[bhs->srcId_] = c;

  bhs->opCode_ = AGENT_RESPONCE;
  bhs->retCode_ = AGENT_E_EXEC_SUCESS;
  iov.iov_base = bhs;
  iov.iov_len = sizeof(AgentBhs);
  SendMsg(c->fd(), &iov, 1, MSG_NOSIGNAL | MSG_WAITALL);
  c->rxWanted_ = 0;
  c->rxRecved_ = 0;
}

void AgentServer::handleAgentRequest(ConnectionPtr c)
{
  AgentBhs *bhs = static_cast<AgentBhs *>(c->rxIov_[0].iov_base);
  auto itor = activeConns_.find(bhs->dstId_);
  if (itor != activeConns_.end()) {
    ConnectionPtr peer = (ConnectionPtr)itor->second;
    MSF_INFO << "\n Send cmd to"
              << "\n cmd: " << bhs->cmd_ 
              << "\n cid: " << peer->cid_;
    //http://blog.chinaunix.net/uid-14949191-id-3967282.html
    // std::swap(bhs->srcId_, bhs->dstId_);
    {
      AgentAppId tmpId = bhs->srcId_;
      bhs->srcId_ = bhs->dstId_;
      bhs->dstId_ = tmpId;
    }
    struct iovec iov[2];
    iov[0].iov_base = bhs;
    iov[0].iov_len = sizeof(AgentBhs);
    //runinloop到这个loop中去
    SendMsg(peer->fd(), iov, 1, MSG_NOSIGNAL | MSG_WAITALL);
  } else {
    MSF_INFO << "\n Peer offline"
              << "\n cmd: " << bhs->cmd_;
    struct iovec iov;
    bhs->restLen_ = 0;
    bhs->dataLen_ = 0;
    // int temp = static_cast<int>(bhs->cmd_);
    // temp++;
    // bhs->cmd_ = static_cast<AgentCommand>(temp);
    bhs->opCode_ = AGENT_RESPONCE;
    bhs->retCode_ = AGENT_E_PEER_OFFLINE;
    iov.iov_base = bhs;
    iov.iov_len = sizeof(AgentBhs);
    SendMsg(c->fd(), &iov, 1, MSG_NOSIGNAL | MSG_WAITALL);
  }
}

void AgentServer::handleAgentBhs(ConnectionPtr c) {
  AgentBhs *bhs = static_cast<AgentBhs *>(c->rxIov_[0].iov_base);
  debugAgentBhs(bhs);

  switch (bhs->cmd_) {
    case AGENT_LOGIN_REQUEST:
      handleAgentLogin(c);
      break;
    default:
      handleAgentRequest(c);
      break;
  }
}

void AgentServer::readConn(ConnectionPtr c) {
  if (unlikely(c->fd() <= 0)) {
    return;
  }

  MSF_INFO << "Read conn for fd: " << c->fd();
  if (c->rxWanted_ == 0) {
    c->rxWanted_ = sizeof(AgentBhs);
    AgentBhs *bhs = static_cast<AgentBhs *>(mpool_->alloc(c->rxWanted_));
    if (!bhs) {
      DrianData(c->fd(), 2048);
      return;
    }
    memset(bhs, 0, c->rxWanted_);
    c->rxIov_[0].iov_base = bhs;
    c->rxIov_[0].iov_len = c->rxWanted_;
  }

  if (!handleRxIORet(
          c, RecvMsg(c->fd(), c->rxIov_, 1, MSG_NOSIGNAL | MSG_WAITALL))) {
    return;
  }

  handleAgentBhs(c);
  mpool_->free(c->rxIov_[0].iov_base);
}

void AgentServer::writeConn(ConnectionPtr c) {
  if (unlikely(c->fd() <= 0)) {
    return;
  }
  // MSF_INFO << "Write conn for fd: " << fd_;
}

void AgentServer::freeConn(ConnectionPtr c) {
  if (unlikely(c->fd() <= 0)) {
    return;
  }
  MSF_INFO << "Close conn for fd: " << c->fd();
  std::lock_guard<std::mutex> lock(mutex_);
  c->close();
  freeConns_.push_back(c);
}

/* Callback when acceptor epoll read event hanppen */
void AgentServer::newConn(const int fd, const uint16_t event) {
  std::lock_guard<std::mutex> lock(mutex_);
  /* If single thread to handle accept, so no mutex guard */
  if (unlikely(freeConns_.empty())) {
    // connPerAlloc_
    for (uint32_t i = 0; i < 1; ++i) {
      ConnectionPtr conn = std::make_shared<Connector>();
      if (conn == nullptr) {
        MSF_ERROR << "Fail to alloc connection for fd: " << fd;
        close(fd);
        return;
      }
      freeConns_.push_back(conn);
    }
  }

  ConnectionPtr c = freeConns_.front();
  freeConns_.pop_front();

  MSF_INFO << "Alloc connection for fd: " << fd;

  c->init(stack_->getHashLoop(), fd);
  c->setSuccCallback(std::bind(&AgentServer::succConn, this, c));
  c->setReadCallback(std::bind(&AgentServer::readConn, this, c));
  c->setWriteCallback(std::bind(&AgentServer::writeConn, this, c));
  c->setCloseCallback(std::bind(&AgentServer::freeConn, this, c));
  c->setErrorCallback(std::bind(&AgentServer::freeConn, this, c));
  c->enableEvent();
  return;
}

bool AgentServer::initListen() {
  for (uint32_t i = 0; i < 1; ++i) {
    InetAddress addr(srvConf_.ipAddr4_.c_str(), srvConf_.tcpPort_, AF_INET,
                     SOCK_STREAM);
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
    e->setErrorCallback(std::bind(&Acceptor::errorCb, actor));
    e->setCloseCallback(std::bind(&Acceptor::errorCb, actor));
    e->disableWriting();
    e->enableClosing();
    e->enableReading();
    actors_.push_back(actor);
  }

  return true;
}

void AgentServer::init() {
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
  srv.init();
  srv.start();
  return 0;
}
