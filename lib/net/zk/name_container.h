#ifndef USTEVENT_NAME_CONTAINER_H_
#define USTEVENT_NAME_CONTAINER_H_

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include "one_zk_container.h"

using namespace MSF;

namespace MSF {

class NameContainer {
 public:
  NameContainer(int timeout = 15000, const std::string& zk_log = std::string(),
                bool zk_log_debug = false);
  ~NameContainer();

  int AddZkPath(const std::string& zk_server, const std::string& path);

  int SetSessionExpiredCb(const std::string& zk_server,
                          const SessionStateChangeCb& cb);
  int SetSessionExpired2ConnectedCb(const std::string& zk_server,
                                    const SessionStateChangeCb& cb);
  int SetSessionConnectedCb(const std::string& zk_server,
                            const SessionStateChangeCb& cb);
  int SetSessionConnectingCb(const std::string& zk_server,
                             const SessionStateChangeCb& cb);

  int GetNNCForPath(const std::string& zk_server, const std::string& path,
                    ucloud::uns::NameNodeContent& nnc,
                    int timeout = 1000);  // 单位ms
  int GetNNCForPath(const std::string& zk_server, const std::string& path,
                    const GetNNCCb& cb);

  int GetNNCForNode(const std::string& zk_server, const std::string& path,
                    const std::string& node_name,
                    ucloud::uns::NameNodeContent& nnc,
                    int timeout = 1000);  // 单位ms
  int GetNNCForNode(const string& zk_server, const string& path,
                    const string& node_name, const GetNNCCb& cb);

  int GetAllNNCForPath(
      const std::string& zk_server, const std::string& path,
      std::map<std::string, ucloud::uns::NameNodeContent>& node_to_nnc,
      int timeout = 1000);  // 单位ms
  int GetAllNNCForPath(const std::string& zk_server, const std::string& path,
                       const GetAllNNCCb& cb);

  int RegisterInstance(const std::string& zk_server,
                       const std::string& instance, const std::string& data);

  int UnRegisterInstance(const std::string& zk_server,
                         const std::string& instance);

  int RegisterLeader(const std::string& zk_server, const std::string& leader,
                     const std::string& data, WatchLeaderCb& cb, void* arg);
  void NameContainerConnectToServer(const string& zk_server);
  TimerId ResetReconnectTimer(const string& zk_server);
  bool SetReconnectTimer(const string& zk_server, TimerId timer_id);

 private:
  mutable std::mutex mutex_;  // 保护zk_container_map_
  std::map<std::string, std::shared_ptr<OneZkContainer> > zk_container_map_;
  int timeout_;
  std::string zk_log_;
  bool zk_log_debug_;
  FILE* log_;
};

}  // namespace

#endif
