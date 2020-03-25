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
#ifndef __MSF_AGENT_SERVER_H__
#define __MSF_AGENT_SERVER_H__

#include <base/Noncopyable.h>
#include <base/Plugin.h>
#include <base/ThreadPool.h>
#include <base/mem/Buffer.h>
#include <base/mem/MemPool.h>
#include <client/AgentConn.h>
#include <event/Event.h>
#include <event/EventLoop.h>
#include <event/EventStack.h>
#include <proto/AgentProto.h>
#include <proto/Protocol.h>
#include <sock/Acceptor.h>

using namespace MSF::BASE;
using namespace MSF::SOCK;
using namespace MSF::PLUGIN;
using namespace MSF::EVENT;
using namespace MSF::AGENT;

namespace MSF {
namespace AGENT {

#define DEFAULT_QUEUE_SIZE (32)
#define DEFAULT_UNIX_PATH "/var/tmp/AgentServer.sock"
#define DEFAULT_CONF_PATH "/home/luotang.me/conf/AgentServer.conf"
#define DEFAULT_LOG_PATH "/home/luotang.me/log/AgentServer.log"

class AgentServer : public Noncopyable {
 public:
  AgentServer();
  ~AgentServer();

  void init(int argc, char **argv);
  void start();

 private:
  bool verifyAgentBhs(const Agent::AgentBhs &bhs);
  void handleAgentPdu(AgentConnPtr c, struct iovec &head);
  void handleAgentLogin(AgentConnPtr c, Agent::AgentBhs &bhs,
                        struct iovec &head);
  void handleAgentRequest(AgentConnPtr c, Agent::AgentBhs &bhs,
                          struct iovec &head);

  void succConn(AgentConnPtr c);
  void readConn(AgentConnPtr c);
  void writeConn(AgentConnPtr c);
  void freeConn(AgentConnPtr c);
  void newConn(const int fd, const uint16_t event);

  void showUsage();
  void parseOption(int argc, char **argv);
  bool initConfig();
  bool initListen();

 private:
  std::string version_;
  std::string confFile_;
  bool daemon_;
  int logLevel_;
  std::string logFile_;
  std::string pidFile_;
  std::vector<std::string> pluginsList_;

  std::string ipAddr4_;
  std::string ipAddr6_;
  int tcpPort_;
  int udpPort_;
  int backLog_;
  int maxConns_; /* Max online client, different from backlog */
  int perConnsAlloc_;
  std::string unixPath_;
  std::string unixMask_;

  struct {
    bool timeout_ : 1;
    bool close_ : 1; /*conn closed state*/
    bool sendFile_ : 1;
    bool sndLowat_ : 1;
    bool tcpNoDelay_ : 2; /* Unix socket default disable */
    bool tcpNoPush_ : 2;
    bool keepAlive_ : 1;
  } opt;
  std::string zkAddr_;
  int zkPort_;

  /*
   * Each thread instance has a wakeup pipe, which other threads
   * can use to signal that they've put a new connection on its queue.
   */
  int maxQueue_;
  int maxThread_;
  bool authChap_;
  std::string packType_;

  enum SocketFdType {
    UNIX_SOCKET,
    TCP_SOCKET_V4,
    TCP_SOCKET_V6,
    UDP_SOCKET_V4,
    UDP_SOCKET_V6,
    SOCKET_TYPEMAX,
  };

  std::mutex mutex_;
  /* If accept in sigle thread, no need to use mutex guard activeConns_ */
  bool accpetSafe_ = true;
  /* Mutiple connections supported, such as tcp, udp, unix, event fd and etc*/
  std::map<Agent::AppId, AgentConnPtr> activeConns_;
  std::list<AgentConnPtr> freeConns_;
  std::atomic_uint64_t connId_; /* increment connection id for server register*/

  bool started_;
  bool exiting_;

  pid_t pid_;

  uint32_t maxFds_;
  uint32_t maxBytes_; /* 64*1024*1025 64MB */
  uint32_t maxCores_;
  uint32_t usedCores_;

  uint32_t maxCmds_;
  uint32_t activeCmds_;
  uint32_t failCmds_;

  std::list<struct AgentCmd *> freeCmdList_;

  PluginManager *pluginMGR_;

  MemPool *mpool_;
  ThreadPool *pool;
  EventStack *stack_;

  std::list<AcceptorPtr> actors_;

  AgentProtoPtr proto_;
};

}  // namespace AGENT
}  // namespace MSF
#endif