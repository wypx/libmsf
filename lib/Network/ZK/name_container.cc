#include "name_container.h"
#include "Logger.h"

using namespace MSF;

namespace MSF {

NameContainer::NameContainer(int timeout, const std::string& zk_log,
                             bool zk_log_debug)
    : timeout_(timeout), zk_log_(zk_log), zk_log_debug_(zk_log_debug) {
  ZooLogLevel log_level =
      zk_log_debug_ ? ZOO_LOG_LEVEL_DEBUG : ZOO_LOG_LEVEL_INFO;
  zoo_set_debug_level(log_level);
  if (!zk_log_.empty()) {
    log_ = fopen(zk_log_.c_str(), "w");
    if (!log_) {
      LOG_FATAL << "can not open zookeeper log file " << zk_log_;
    }
    zoo_set_log_stream(log_);
  }
}

NameContainer::~NameContainer() {
  if (!log_) {
    fclose(log_);
  }
}

int NameContainer::AddZkPath(const std::string& zk_server,
                             const std::string& path) {
  std::unique_lock<std::mutex> lock(mutex_);  // 加锁
  auto container_it = zk_container_map_.find(zk_server);
  if (container_it == zk_container_map_.end()) {
    LOG_INFO << "create zookeeper container for zk_server: " << zk_server;
    zk_container_map_[zk_server].reset(new OneZkContainer(zk_server, timeout_));
    // 同步等待one_zk_container 初始化完成
    int ret = zk_container_map_[zk_server]->Init();
    if (ret != 0) {
      LOG_ERROR << "zookeeper container cannot connect to zookeeper server.";
      return ret;
    }
  } else if (!container_it->second->init_ready()) {
    int ret = container_it->second->Init();
    if (ret != 0) {
      LOG_ERROR << "zookeeper container cannot connect to zookeeper server.";
      return ret;
    }
  }
  auto& container = zk_container_map_[zk_server];
  container->AddZkPath(path);
  // 触发一次路径下的节点获取
  container->GetChildrenNodeForPath(path.c_str());
  return 0;
}

int NameContainer::SetSessionExpiredCb(const std::string& zk_server,
                                       const SessionStateChangeCb& cb) {
  std::unique_lock<std::mutex> lock(mutex_);  // 加锁
  auto container_it = zk_container_map_.find(zk_server);
  if (container_it != zk_container_map_.end()) {
    if (container_it->second->init_ready()) {
      // 如果one zookeeper container已经初始化好再设置回调，之前的回调可能丢失
      LOG_WARN << "zookeeper container for zk_server already init, zk_server:"
               << zk_server;
    }
  } else {
    LOG_INFO << "create zookeeper container for zk_server: " << zk_server;
    zk_container_map_[zk_server].reset(new OneZkContainer(zk_server, timeout_));
  }
  auto& container = zk_container_map_[zk_server];
  container->set_session_expired_cb(cb);
  return 0;
}

int NameContainer::SetSessionExpired2ConnectedCb(
    const std::string& zk_server, const SessionStateChangeCb& cb) {
  std::unique_lock<std::mutex> lock(mutex_);  // 加锁
  auto container_it = zk_container_map_.find(zk_server);
  if (container_it != zk_container_map_.end()) {
    if (container_it->second->init_ready()) {
      // 如果one zookeeper container已经初始化好再设置回调，之前的回调可能丢失
      LOG_WARN << "zookeeper container for zk_server already init, zk_server:"
               << zk_server;
    }
  } else {
    LOG_INFO << "create zookeeper container for zk_server: " << zk_server;
    zk_container_map_[zk_server].reset(new OneZkContainer(zk_server, timeout_));
  }
  auto& container = zk_container_map_[zk_server];
  container->set_session_expired_2_connected_cb(cb);
  return 0;
}

int NameContainer::SetSessionConnectedCb(const std::string& zk_server,
                                         const SessionStateChangeCb& cb) {
  std::unique_lock<std::mutex> lock(mutex_);  // 加锁
  auto container_it = zk_container_map_.find(zk_server);
  if (container_it != zk_container_map_.end()) {
    if (container_it->second->init_ready()) {
      // 如果one zookeeper container已经初始化好再设置回调，之前的回调可能丢失
      LOG_WARN << "zookeeper container for zk_server already init, zk_server:"
               << zk_server;
    }
  } else {
    LOG_INFO << "create zookeeper container for zk_server: " << zk_server;
    zk_container_map_[zk_server].reset(new OneZkContainer(zk_server, timeout_));
  }
  auto& container = zk_container_map_[zk_server];
  container->set_session_connected_cb(cb);
  return 0;
}

int NameContainer::SetSessionConnectingCb(const std::string& zk_server,
                                          const SessionStateChangeCb& cb) {
  std::unique_lock<std::mutex> lock(mutex_);  // 加锁
  auto container_it = zk_container_map_.find(zk_server);
  if (container_it != zk_container_map_.end()) {
    if (container_it->second->init_ready()) {
      // 如果one zookeeper container已经初始化好再设置回调，之前的回调可能丢失
      LOG_WARN << "zookeeper container for zk_server already init, zk_server:"
               << zk_server;
    }
  } else {
    LOG_INFO << "create zookeeper container for zk_server: " << zk_server;
    zk_container_map_[zk_server].reset(new OneZkContainer(zk_server, timeout_));
  }
  auto& container = zk_container_map_[zk_server];
  container->set_session_connecting_cb(cb);
  return 0;
}

int NameContainer::GetNNCForPath(const string& zk_server, const string& path,
                                 ucloud::uns::NameNodeContent& nnc,
                                 int timeout) {
  std::unique_lock<std::mutex> lock(mutex_);  // 加锁
  if (zk_container_map_.find(zk_server) == zk_container_map_.end()) {
    LOG_ERROR << "zookeeper container for zk_server not exist, zk_server: "
              << zk_server;
    return -1;
  }
  auto& container = zk_container_map_[zk_server];
  int ret = container->GetNNCForPath(path, nnc, timeout);
  return ret;
}

int NameContainer::GetNNCForPath(const std::string& zk_server,
                                 const std::string& path, const GetNNCCb& cb) {
  std::unique_lock<std::mutex> lock(mutex_);  // 加锁
  if (zk_container_map_.find(zk_server) == zk_container_map_.end()) {
    LOG_ERROR << "zookeeper container for zk_server not exist, zk_server: "
              << zk_server;
    return -1;
  }
  auto& container = zk_container_map_[zk_server];
  int ret = container->GetNNCForPath(path, cb);
  return ret;
}

int NameContainer::GetNNCForNode(const string& zk_server, const string& path,
                                 const string& node_name,
                                 ucloud::uns::NameNodeContent& nnc,
                                 int timeout) {
  std::unique_lock<std::mutex> lock(mutex_);  // 加锁
  if (zk_container_map_.find(zk_server) == zk_container_map_.end()) {
    LOG_ERROR << "zookeeper container for zk_server not exist, zk_server: "
              << zk_server;
    return -1;
  }
  auto& container = zk_container_map_[zk_server];
  int ret = container->GetNNCForNode(path, node_name, nnc, timeout);
  return ret;
}

int NameContainer::GetNNCForNode(const string& zk_server, const string& path,
                                 const string& node_name, const GetNNCCb& cb) {
  std::unique_lock<std::mutex> lock(mutex_);  // 加锁
  if (zk_container_map_.find(zk_server) == zk_container_map_.end()) {
    LOG_ERROR << "zookeeper container for zk_server not exist, zk_server: "
              << zk_server;
    return -1;
  }
  auto& container = zk_container_map_[zk_server];
  int ret = container->GetNNCForNode(path, node_name, cb);
  return ret;
}

int NameContainer::GetAllNNCForPath(
    const string& zk_server, const string& path,
    std::map<std::string, ucloud::uns::NameNodeContent>& node_to_nnc,
    int timeout) {
  std::unique_lock<std::mutex> lock(mutex_);  // 加锁
  if (zk_container_map_.find(zk_server) == zk_container_map_.end()) {
    LOG_ERROR << "zookeeper container for zk_server not exist, zk_server: "
              << zk_server;
    return -1;
  }
  auto& container = zk_container_map_[zk_server];
  int ret = container->GetAllNNCForPath(path, node_to_nnc, timeout);
  return ret;
}

int NameContainer::GetAllNNCForPath(const std::string& zk_server,
                                    const std::string& path,
                                    const GetAllNNCCb& cb) {
  std::unique_lock<std::mutex> lock(mutex_);  // 加锁
  if (zk_container_map_.find(zk_server) == zk_container_map_.end()) {
    LOG_ERROR << "zookeeper container for zk_server not exist, zk_server: "
              << zk_server;
    return -1;
  }
  auto& container = zk_container_map_[zk_server];
  int ret = container->GetAllNNCForPath(path, cb);
  return ret;
}

int NameContainer::RegisterInstance(const std::string& zk_server,
                                    const std::string& instance,
                                    const std::string& data) {
  std::unique_lock<std::mutex> lock(mutex_);  // 加锁
  auto container_it = zk_container_map_.find(zk_server);
  if (container_it == zk_container_map_.end()) {
    LOG_INFO << "create zookeeper container for zk_server: " << zk_server;
    zk_container_map_[zk_server].reset(new OneZkContainer(zk_server, timeout_));
    // 同步等待one_zk_container 初始化完成
    int ret = zk_container_map_[zk_server]->Init();
    if (ret != 0) {
      LOG_ERROR << "zookeeper container cannot connect to zookeeper server.";
      return ret;
    }
  } else if (!container_it->second->init_ready()) {
    int ret = container_it->second->Init();
    if (ret != 0) {
      LOG_ERROR << "zookeeper container cannot connect to zookeeper server.";
      return ret;
    }
  }
  auto& container = zk_container_map_[zk_server];
  int ret = container->RegisterInstance(instance, data);
  return ret;
}

int NameContainer::UnRegisterInstance(const std::string& zk_server,
                                      const std::string& instance) {
  std::unique_lock<std::mutex> lock(mutex_);  // 加锁
  auto container_it = zk_container_map_.find(zk_server);
  if (container_it == zk_container_map_.end() ||
      !container_it->second->init_ready()) {
    LOG_ERROR
    << "zookeeper conntanier must be init ready when unreisger instance,"
    << " zk_server:" << zk_server << " instance:" << instance;
    return -1;
  }
  int ret = container_it->second->UnRegisterInstance(instance);
  return ret;
}

int NameContainer::RegisterLeader(const std::string& zk_server,
                                  const std::string& leader,
                                  const std::string& data, WatchLeaderCb& cb,
                                  void* arg) {
  std::unique_lock<std::mutex> lock(mutex_);  // 加锁
  auto container_it = zk_container_map_.find(zk_server);
  if (container_it == zk_container_map_.end()) {
    LOG_INFO << "create zookeeper container for zk_server: " << zk_server;
    zk_container_map_[zk_server].reset(new OneZkContainer(zk_server, timeout_));
    // 同步等待one_zk_container 初始化完成
    int ret = zk_container_map_[zk_server]->Init();
    if (ret != 0) {
      LOG_ERROR << "zookeeper container cannot connect to zookeeper server.";
      return ret;
    }
  } else if (!container_it->second->init_ready()) {
    int ret = container_it->second->Init();
    if (ret != 0) {
      LOG_ERROR << "zookeeper container cannot connect to zookeeper server.";
      return ret;
    }
  }
  auto& container = zk_container_map_[zk_server];
  int ret = container->RegisterLeader(leader, data, cb, arg);
  return ret;
}

void NameContainer::NameContainerConnectToServer(const string& zk_server) {
  auto container_it = zk_container_map_.find(zk_server);
  if (container_it == zk_container_map_.end()) {
    LOG_WARN << "zookeeper container could not find for zk server: "
             << zk_server;  // could not happen in theory.
  } else {
    container_it->second->reset_reconnect_timer();
    container_it->second->reset_status();
    container_it->second->ConnectToServer();
  }
}

TimerId NameContainer::ResetReconnectTimer(const string& zk_server) {
  auto container_it = zk_container_map_.find(zk_server);
  if (container_it == zk_container_map_.end()) {
    LOG_WARN << "zookeeper container could not find for zk server: "
             << zk_server;  // could not happen in theory.
    return TimerId();
  }
  return container_it->second->reset_reconnect_timer();
}

bool NameContainer::SetReconnectTimer(const string& zk_server,
                                      TimerId timer_id) {
  auto container_it = zk_container_map_.find(zk_server);
  if (container_it == zk_container_map_.end()) {
    LOG_WARN << "zookeeper container could not find for zk server: "
             << zk_server;  // could not happen in theory.
    return false;
  }
  return container_it->second->set_reconnect_timer(timer_id);
}

}  // namespace MSF
