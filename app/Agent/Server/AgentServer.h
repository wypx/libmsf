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
#ifndef AGENT_SERVER_AGENTSERVER_H
#define AGENT_SERVER_AGENTSERVER_H

#include "AgentConn.h"
#include "Noncopyable.h"
#include "Plugin.h"
#include "ThreadPool.h"
#include "Buffer.h"
#include "MemPool.h"
#include "EventLoop.h"
#include "EventStack.h"
#include "AgentProto.h"
#include "Acceptor.h"

using namespace MSF;

namespace MSF {

static const std::string kDefaultUnixPath = "/var/tmp/AgentServer.sock";
static const std::string kDefaultConfPath = "/home/luotang.me/conf/AgentServer.conf";
static const std::string kDefaultLogDirPath = "/home/luotang.me/log/";

class AgentServer {
 public:
  AgentServer();
  ~AgentServer();

  void Init(int argc, char **argv);
  void Start();

 private:
  bool VerifyAgentBhs(const Agent::AgentBhs &bhs);
  void HandleAgentPdu(AgentConnPtr c, struct iovec &head);
  void HandleAgentLogin(AgentConnPtr c, Agent::AgentBhs &bhs,
                        struct iovec &head);
  void HandleAgentRequest(AgentConnPtr c, Agent::AgentBhs &bhs,
                          struct iovec &head);

  void SuccConn(AgentConnPtr c);
  void ReadConn(AgentConnPtr c);
  void WriteConn(AgentConnPtr c);
  void FreeConn(AgentConnPtr c);
  void NewConn(const int fd, const uint16_t event);

  void DebugInfo();
  void ParseOption(int argc, char **argv);
  bool LoadConfig();
  bool InitNetwork();

 private:
  std::string version_;
  std::string config_file_;
  bool daemon_;
  int log_level_;
  std::string log_dir_;
  std::string pid_file_;
  std::vector<std::string> plugins_;

  std::string ip_addr4_;
  std::string ip_addr6_;
  uint16_t tcp_port_;
  uint16_t udp_port_;
  uint32_t back_log_;
  uint32_t max_conns_; /* Max online client, different from backlog */
  uint32_t per_alloc_conns_;
  std::string unix_path_;
  uint32_t unix_mask_;

  struct {
    bool timeout_ : 1;
    bool close_ : 1; /*conn closed state*/
    bool send_file_ : 1;
    bool ssnd_lowat_ : 1;
    bool tcp_nodelay_ : 2; /* Unix socket default disable */
    bool tcp_nopush_ : 2;
    bool keep_alive_ : 1;
  } opt;
  std::string zk_addr_;
  uint16_t zk_port_;

  /*
   * Each thread instance has a wakeup pipe, which other threads
   * can use to signal that they've put a new connection on its queue.
   */
  int maxQueue_;
  uint32_t work_thread_num_;
  bool agent_auth_chap_;
  std::string agent_pack_type_;

  enum SocketFdType {
    UNIX_SOCKET,
    TCP_SOCKET_V4,
    TCP_SOCKET_V6,
    UDP_SOCKET_V4,
    UDP_SOCKET_V6,
    SOCKET_TYPE_MAX
  };

  std::mutex mutex_;
  /* If accept in sigle thread, no need to use mutex guard activeConns_ */
  bool accpet_safe_ = true;
  /* Mutiple connections supported, such as tcp, udp, unix, event fd and etc*/
  std::map<Agent::AppId, AgentConnPtr> active_conns_;
  std::list<AgentConnPtr> free_conns_;
  std::atomic_uint64_t conn_id_; /* increment connection id for server register*/

  bool started_;
  bool exiting_;

  pid_t pid_;

  uint32_t max_fds_;
  uint32_t max_bytes_; /* 64*1024*1025 64MB */
  uint32_t max_cores_;
  uint32_t used_cores_;

  uint32_t max_cmds_;
  uint32_t active_cmds_;
  uint32_t fail_cmds_;

  std::list<struct AgentCmd *> free_cmd_list_;

  PluginManager *plugin_manager_;

  MemPool *mpool_;
  ThreadPool *pool;
  EventStack *stack_;

  std::list<AcceptorPtr> actors_;

  AgentProtoPtr proto_;
};

}  // namespace MSF
#endif