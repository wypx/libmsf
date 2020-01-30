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

#include <base/MemPool.h>
#include <base/Noncopyable.h>
#include <base/Plugin.h>
#include <base/ThreadPool.h>
#include <event/Event.h>
#include <event/EventLoop.h>
#include <event/EventStack.h>
#include <sock/Acceptor.h>
#include <sock/Connector.h>

using namespace MSF::BASE;
using namespace MSF::SOCK;
using namespace MSF::PLUGIN;
using namespace MSF::EVENT;

namespace MSF {
namespace AGENT {

#define DEFAULT_QUEUE_SIZE (32)
#define DEFAULT_UNIX_PATH "/var/tmp/AgentServer.sock"
#define DEFAULT_CONF_PATH "/home/luotang.me/conf/AgentServer.conf"

/* enum of index available in the agent conf */
enum CONFIG_IDX {
  CONFIG_INVALID = 0,
  CONFIG_LOGLEVEL,
  CONFIG_LOGFILE,
  CONFIG_PIDFILE,
  CONFIG_DAEMON,
  CONFIG_PLUGINS,
  CONFIG_BACKLOG,
  CONFIG_MAXCONNS,
  CONFIG_MAXTHREAD,
  CONFIG_MAXQUEUE,
  CONFIG_IPADDRV4,
  CONFIG_IPADDRV6,
  CONFIG_TCPPORT,
  CONFIG_UDPPORT,
  CONFIG_UNXIPATH,
  CONFIG_UNXIMASK,
  CONFIG_AUTHCHAP,
  CONFIG_PACKTYPE,
  CONFIG_BUTT,
};

struct SrvConfig {
  std::string version_;
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
  std::string unixPath_;
  std::string unixMask_;
  struct {
    bool timeout_ : 1;
    bool close_ : 1; /*conn closed state*/
    bool sendFile_ : 1;
    bool sndLowat_ : 1;
    bool tcpNoDelay_ : 2; /* Unix socket default disable */
    bool tcpNoPush_ : 2;
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
};

class AgentServer : public Noncopyable {
 public:
  AgentServer();
  ~AgentServer();

  void init();
  void start();

 private:
  enum SocketFdType {
    UNIX_SOCKET,
    TCP_SOCKET_V4,
    TCP_SOCKET_V6,
    UDP_SOCKET_V4,
    UDP_SOCKET_V6,
    SOCKET_TYPEMAX,
  };

  std::mutex mutex_;
  /* Mutiple connections supported, such as tcp, udp, unix, event fd and etc*/
  std::map<uint32_t, ConnectionPtr> activeConns_; /* Current connections */
  std::list<ConnectionPtr> freeConns_;
  uint32_t connPerAlloc_;
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
  struct SrvConfig srvConf_;

  MemPool *mpool_;
  ThreadPool *pool;

  EventStack *stack_;

  std::list<std::shared_ptr<Acceptor>> actors_;

  std::string confFile_;
  std::string logFile_;

  bool handleRxIORet(ConnectionPtr c, const int ret);
  bool handleTxIORet(ConnectionPtr c, const int ret);

  void handleAgentBhs(ConnectionPtr c);

  void succConn(ConnectionPtr c);
  void readConn(ConnectionPtr c);
  void writeConn(ConnectionPtr c);
  void freeConn(ConnectionPtr c);
  void newConn(const int fd, const uint16_t event);

  bool initConfig();
  bool initListen();
};

}  // namespace AGENT
}  // namespace MSF
#endif