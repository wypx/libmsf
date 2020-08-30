#include "one_zk_container.h"
#include <unistd.h>
#include <algorithm>
#include <chrono>
#include "../base/logging.h"
#include <string.h>

namespace uevent {

static const char *state2String(int state) {
  if (state == 0) return "CLOSED_STATE";
  if (state == ZOO_CONNECTING_STATE) return "CONNECTING_STATE";
  if (state == ZOO_ASSOCIATING_STATE) return "ASSOCIATING_STATE";
  if (state == ZOO_CONNECTED_STATE) return "CONNECTED_STATE";
  if (state == ZOO_EXPIRED_SESSION_STATE) return "EXPIRED_SESSION_STATE";
  if (state == ZOO_AUTH_FAILED_STATE) return "AUTH_FAILED_STATE";

  return "INVALID_STATE";
}

static const char *type2String(int state) {
  if (state == ZOO_CREATED_EVENT) return "CREATED_EVENT";
  if (state == ZOO_DELETED_EVENT) return "DELETED_EVENT";
  if (state == ZOO_CHANGED_EVENT) return "CHANGED_EVENT";
  if (state == ZOO_CHILD_EVENT) return "CHILD_EVENT";
  if (state == ZOO_SESSION_EVENT) return "SESSION_EVENT";
  if (state == ZOO_NOTWATCHING_EVENT) return "NOTWATCHING_EVENT";

  return "UNKNOWN_EVENT_TYPE";
}

struct WatchEntry {
  std::string path;
  std::string data;
  void *ctx;
  WatchLeaderCb cb;
  void *arg;
  WatchEntry(const std::string &xpath, const std::string &xdata, void *xctx,
             const WatchLeaderCb &xcb, void *xarg)
      : path(xpath), data(xdata), ctx(xctx), cb(xcb), arg(xarg) {}
};

OneZkContainer::OneZkContainer(const std::string &zk_server, int timeout)
    : zk_server_(zk_server), timeout_(timeout), zhandle_(NULL) {
  init_ready_.store(false);
}

OneZkContainer::~OneZkContainer() {
  if (zhandle_) {
    int ret = zookeeper_close(zhandle_);
    if (ret != ZOK) {
      LOG_ERROR << "zookeeper_close failed for zhandle: "
                << reinterpret_cast<uint64_t>(zhandle_)
                << " ret: " << zerror(ret);
    }
  }
}

int OneZkContainer::Init() {
  if (ConnectToServer() == -1) {
    LOG_ERROR << "connect to zookeeper server failed, zk_server:" << zk_server_;
    return -1;
  }
  if (!WaitZkInitReady()) {
    return -1;
  }
  return 0;
}

bool OneZkContainer::WaitZkInitReady() {
  std::unique_lock<std::mutex> lk(mutex_);
  bool succ = cond_.wait_for(lk, std::chrono::milliseconds(1000),
                             [this]() { return init_ready_.load() == true; });
  if (succ) {
    LOG_INFO << "zookeeper init success";
  } else {
    LOG_ERROR << "zookeeper init timeout";
  }
  return succ;
}

void OneZkContainer::NotifyZkInitReady() {
  std::lock_guard<std::mutex> lk(mutex_);
  init_ready_.store(true);
  cond_.notify_all();
}

bool OneZkContainer::init_ready() { return init_ready_.load(); }

int OneZkContainer::ConnectToServer() {
  std::unique_lock<std::mutex> tryLock(connectionMutex_, std::try_to_lock);
  if (tryLock.owns_lock()) {
    LOG_INFO << "start to connect to zookeeper server.";
    return ConnectToServerInternal();
  } else {  // another thread is trying to connect to zookeeper server now.
    LOG_INFO << "zookeeper connection is on the fly.";
    return -1;
  }
}

void OneZkContainer::reset_status() {
  std::unique_lock<std::mutex> lock(mutex_);
  SetAllNodeDirty();
  for (auto &it : instance_registered_) {
    it.second = false;
  }
}

int OneZkContainer::ConnectToServerInternal() {
  if (zhandle_ != NULL) {
    int ret = zookeeper_close(zhandle_);
    if (ret != ZOK) {
      LOG_ERROR << "zookeeper_close failed for zhandle: "
                << reinterpret_cast<uint64_t>(zhandle_)
                << " ret: " << zerror(ret);
    }
    zhandle_ = NULL;
  }
  last_session_state_ = ZOO_EXPIRED_SESSION_STATE;
  init_ready_.store(false);  // zookeeper init之前设置成false
  zhandle_ = zookeeper_init(zk_server_.c_str(), ZkInitWatcher, timeout_, NULL,
                            this, 0);
  LOG_INFO << "zookeeper_init trying to connect to server: "
           << zk_server_.c_str()
           << " zhandle_: " << reinterpret_cast<uint64_t>(zhandle_);
  if (zhandle_ == NULL) {  // 这里没有立即重试，防止疯狂重试
    LOG_ERROR << "connect to zookeeper sever failed, zookeeper server: "
              << zk_server_ << " Error: " << strerror(errno);
    return -1;
  }
  return 0;
}

void OneZkContainer::ZkInitWatcher(zhandle_t *zh, int type, int state,
                                   const char *path, void *watcherCtx) {
  OneZkContainer *pobj = reinterpret_cast<OneZkContainer *>(watcherCtx);
  if (zh != pobj->zhandle_) {
    LOG_ERROR << "zhandle mismatch. please check log. zookeeper handle:"
              << reinterpret_cast<uint64_t>(zh) << " watcher context handle:"
              << reinterpret_cast<uint64_t>(pobj->zhandle_);
    return;
  }
  if (type == ZOO_SESSION_EVENT) {
    if (state == ZOO_CONNECTED_STATE) {
      LOG_INFO << "connect to zookeeper server success! zkHandle:"
               << reinterpret_cast<uint64_t>(pobj->zhandle_)
               << " session id: " << zoo_client_id(pobj->zhandle_)->client_id;
      pobj->NotifyZkInitReady();
      // 从ZOO_CONNECTING_STATE到ZOO_CONNECTED_STATE触发回调
      if (pobj->last_session_state_ == ZOO_CONNECTING_STATE &&
          pobj->session_connected_cb_) {
        LOG_WARN << "zookeeper session change from CONNECTING to CONNECTED,"
                 << " zkHandle:" << reinterpret_cast<uint64_t>(pobj->zhandle_)
                 << " session id: " << zoo_client_id(pobj->zhandle_)->client_id;
        pobj->session_connected_cb_();
      }
      if (pobj->last_session_state_ == ZOO_EXPIRED_SESSION_STATE &&
          pobj->session_expired_2_connected_cb_) {
        LOG_WARN
        << "zookeeper session change from EXPIRED to CONNECTED, zkHandle:"
        << reinterpret_cast<uint64_t>(pobj->zhandle_)
        << " session id: " << zoo_client_id(pobj->zhandle_)->client_id;
        pobj->session_expired_2_connected_cb_();
      }
      pobj->last_session_state_ = ZOO_CONNECTED_STATE;
      std::unique_lock<std::mutex> lock(pobj->mutex_);
      pobj->GetChildrenNode();  // 重新watch
      return;
    } else if (state == ZOO_CONNECTING_STATE) {
      LOG_WARN << "zookeeper session change from "
               << state2String(pobj->last_session_state_)
               << " to CONNECTING, zkHandle:"
               << reinterpret_cast<uint64_t>(pobj->zhandle_)
               << " session id:" << zoo_client_id(pobj->zhandle_)->client_id;
      if (pobj->session_connecting_cb_) {
        pobj->session_connecting_cb_();
      }
      pobj->last_session_state_ = ZOO_CONNECTING_STATE;
      return;
    } else if (state == ZOO_EXPIRED_SESSION_STATE) {
      LOG_ERROR << "zookeeper session expired for zhandle:"
                << reinterpret_cast<uint64_t>(pobj->zhandle_)
                << " try to setup new connection...";
      pobj->reset_status();
      // 放到重新connect之前设置，因为如果一直connecting,
      // 外部会调用ConnectToServer
      // pobj->last_session_state_ = ZOO_EXPIRED_SESSION_STATE;
      pobj->ConnectToServer();
      // 通知业务会话发生过期重连, 在锁外, 防止业务又加锁后死锁
      if (pobj->session_expired_cb_) {
        pobj->session_expired_cb_();
      }
      return;
    }
  }
  LOG_ERROR << "zookeeper watcher unhandled event: " << type2String(type)
            << " state: " << state2String(state);
}

void OneZkContainer::AddZkPath(const std::string &zk_path) {
  LOG_DEBUG << "add zookeeper path: " << zk_path;
  if (path_to_nodes_.find(zk_path) != path_to_nodes_.end()) {
    LOG_INFO << "zookeeper path exist, path: " << zk_path
             << " zk_server: " << zk_server_;
    return;
  }
  path_to_nodes_[zk_path].clear();
  cache_state_[zk_path].has_data = false;
  cache_state_[zk_path].is_dirty = true;
  cache_state_[zk_path].error_flag = false;
}

void OneZkContainer::CallCb(int ret, const std::string &path,
                            std::unique_lock<std::mutex> &lock) {
  if (ret == 0) {
    cache_state_[path].has_data = true;
  }
  ucloud::uns::NameNodeContent nnc;
  for (auto &cb : path_cb_list_[path]) {
    lock.unlock();
    LOG_INFO << "zookeeper get path cb called, path:" << path;
    ret = GetNNCForPath(path, nnc, 0);  // 调用同步接口
    cb(ret, nnc);
    lock.lock();
  }
  path_cb_list_[path].clear();

  for (auto &it : node_cb_list_) {
    if (it.first.first == path) {
      for (auto &cb : it.second) {
        lock.unlock();
        LOG_INFO << "zookeeper get node cb called, path:" << path
                 << " node:" << it.first.second;
        ret = GetNNCForNode(it.first.first, it.first.second, nnc, 0);
        cb(ret, nnc);
        lock.lock();
      }
      it.second.clear();
    }
  }
  std::map<std::string, ucloud::uns::NameNodeContent> node_to_nnc;
  for (auto &cb : all_nnc_cb_list_[path]) {
    lock.unlock();
    LOG_INFO << "zookeeper get all nnc cb called, path:" << path;
    ret = GetAllNNCForPath(path, node_to_nnc, 0);  // 调用同步接口
    cb(ret, node_to_nnc);
    lock.lock();
  }
  all_nnc_cb_list_[path].clear();
}

//获取所有path下的所有node
int OneZkContainer::GetChildrenNode() {
  int ret = 0;
  for (auto it = path_to_nodes_.begin(); it != path_to_nodes_.end(); it++) {
    ret = GetChildrenNodeForPath(it->first.c_str());
    if (ret == -1) {
      LOG_ERROR << "zookeeper get children node for path error: " << it->first;
      continue;  //即使出错也会继续
    }
  }
  return 0;
}

//获取指定path下的所有node
int OneZkContainer::GetChildrenNodeForPath(const char *path) {
  if (zhandle_ == NULL) {
    LOG_WARN << "zookeeper zhandle is null when get GetChildrenNodeForPath";
    ConnectToServer();
  }
  string path_str(path);
  GetChildrenCtx *get_children_ctx = new GetChildrenCtx({this, path_str});
  int ret = zoo_awget_children(zhandle_, path, ChildrenWatcher, this,
                               GetChildenCompleteCb, get_children_ctx);
  if (ret != ZOK) {
    LOG_ERROR << "zookeeper get children error for path: " << path << " "
              << zerror(ret);
    cache_state_[path].is_dirty = true;
    return -1;
  }
  return 0;
}

void OneZkContainer::GetChildenCompleteCb(int rc,
                                          const struct String_vector *str_vec,
                                          const void *data) {
  OneZkContainer *pobj =
      reinterpret_cast<OneZkContainer *>(((GetChildrenCtx *)data)->ctx);
  string path = ((GetChildrenCtx *)data)->path;
  delete (GetChildrenCtx *)data;
  std::unique_lock<std::mutex> lock(pobj->mutex_);  // 防止zk线程和其他线程竞争
  if (rc != ZOK) {
    LOG_ERROR << "zookeeper get children error, for path: " << path << " "
              << zerror(rc) << " set cache dirty";
    auto &cache_state = pobj->cache_state_[path];
    cache_state.is_dirty = true;
    // 设置为错误数据，使同步等待能快速退出
    cache_state.error_flag = true;
    pobj->CallCb(-1, path, lock);
    pobj->cond_.notify_all();
    return;
  }
  if (str_vec->count == 0) {  // 没有节点
    LOG_ERROR << "zookeeper no node in the path:" << path << " set cache dirty";
    auto &cache_state = pobj->cache_state_[path];
    cache_state.is_dirty = true;
    // 设置为错误数据，使同步等待能快速退出
    cache_state.error_flag = true;
    pobj->CallCb(-1, path, lock);
    pobj->cond_.notify_all();
    return;
  }
  pobj->path_to_nodes_[path].clear();  // 删除原来的该path下的所有node
  for (int i = 0; i < str_vec->count; i++) {
    pobj->path_to_nodes_[path].emplace_back(string(str_vec->data[i]));
  }
  //获取到path下的node后需要获取node下的数据, 只要有一个节点失败就认为失败
  auto &state = pobj->path_to_get_node_data_state_[path];
  state.all_node_success = true;
  LOG_INFO << "zookeeper get node data begin, path:" << path
           << " inflight_node_count: " << state.inflight_node_count;

  vector<string> &node_vec = pobj->path_to_nodes_[path];
  string node_path;
  for (auto it = node_vec.begin(); it != node_vec.end(); it++) {
    node_path = path + "/" + *it;
    if (pobj->GetDataForNode(node_path.c_str()) == -1) {
      LOG_ERROR << "zookeeper get data for node error, node: " << node_path
                << " sert cache dirty";
      auto &cache_state = pobj->cache_state_[path];
      cache_state.is_dirty = true;
      // 设置为错误数据，使同步等待能快速退出
      cache_state.error_flag = true;
      pobj->CallCb(-1, path, lock);
      pobj->cond_.notify_all();
      state.all_node_success = false;
    }
  }
}

void OneZkContainer::SetAllNodeDirty() {
  for (auto it = cache_state_.begin(); it != cache_state_.end(); it++) {
    it->second.is_dirty = true;
  }
}

int OneZkContainer::GetDataForNode(const char *node) {
  GetNodeDataCtx *get_node_data_ctx = new GetNodeDataCtx({this, node});
  int ret = zoo_awget(zhandle_, node, NodeDataWatcher, this, DataCompleteCb,
                      get_node_data_ctx);
  if (ret != ZOK) {
    LOG_ERROR << "zookeeper get node data error, for node: " << node << " "
              << zerror(ret);
    return -1;
  }
  string node_str(node);
  size_t found = node_str.find_last_of("/");
  string path = node_str.substr(0, found);
  auto &state = path_to_get_node_data_state_[path];
  state.inflight_node_count++;
  LOG_INFO << "zookeeper after get data for node:" << node_str
           << " inflight_node_count:" << state.inflight_node_count;
  return 0;
}

void OneZkContainer::DataCompleteCb(int rc, const char *value, int value_len,
                                    const struct Stat *stat, const void *data) {
  OneZkContainer *pobj =
      reinterpret_cast<OneZkContainer *>(((GetNodeDataCtx *)data)->ctx);
  string node = ((GetNodeDataCtx *)data)->node;
  delete (GetNodeDataCtx *)data;
  size_t found = node.find_last_of("/");
  string path = node.substr(0, found);
  std::unique_lock<std::mutex> lock(pobj->mutex_);  // 防止zk线程和其他线程竞争
  auto &state = pobj->path_to_get_node_data_state_[path];
  if (rc != ZOK) {
    state.inflight_node_count--;
    state.all_node_success = false;
    LOG_ERROR << "zookeeper get node data error, node:" << node << " "
              << zerror(rc) << " set cache dirty, inflight_node_count:"
              << state.inflight_node_count;
    auto &cache_state = pobj->cache_state_[path];
    cache_state.is_dirty = true;
    // 设置为错误数据，使同步等待能快速退出
    cache_state.error_flag = true;
    pobj->CallCb(-1, path, lock);
    pobj->cond_.notify_all();
    return;
  }
  ucloud::uns::NameNodeContent nnc;
  string nnc_str(value, value_len);
  nnc.ParseFromString(nnc_str);
  pobj->node_to_nnc_[node] = nnc;
  state.inflight_node_count--;
  LOG_INFO << "zookeeper get data for node: " << node << " ip: " << nnc.ip()
           << " port: " << nnc.port()
           << " inflight_node_count: " << state.inflight_node_count;
  if (state.inflight_node_count == 0 && state.all_node_success == true) {
    state.all_node_success = false;
    pobj->cache_state_[path].is_dirty = false;
    LOG_INFO << "zookeeper set cache dirty false, CallCb success";
    pobj->CallCb(0, path, lock);
    pobj->cond_.notify_all();
  }
}

//监控子节点数据变化
void OneZkContainer::NodeDataWatcher(zhandle_t *zh, int type, int state,
                                     const char *path, void *watcherCtx) {
  string path_str(path);
  OneZkContainer *pobj = reinterpret_cast<OneZkContainer *>(watcherCtx);
  std::unique_lock<std::mutex> lock(pobj->mutex_);  // 防止zk线程和其他线程竞争
  if (type == ZOO_CHANGED_EVENT) {
    LOG_INFO << "zookeeper node data changed for node path: " << path;
    pobj->GetDataForNode(path);
  } else if (type == ZOO_DELETED_EVENT) {  // node 删除了，需要删除对应的nnc
    if (pobj->node_to_nnc_.find(path_str) != pobj->node_to_nnc_.end()) {
      LOG_INFO << "zookeeper ZOO_DELETED_EVENT for node path" << path;
      /**这里不要删除, 节点的选取使用的是path_to_nodes_, 这个只是记录节点信息
        防止当缓存为脏，有节点而没有信息的情况**/
      // pobj->node_to_nnc_.erase(path_str);
    }
  }
}

//监控某个路径子节点的变化
void OneZkContainer::ChildrenWatcher(zhandle_t *zh, int type, int state,
                                     const char *path, void *watcherCtx) {
  OneZkContainer *pobj = reinterpret_cast<OneZkContainer *>(watcherCtx);
  std::unique_lock<std::mutex> lock(pobj->mutex_);  // 防止zk线程和其他线程竞争
  if (ZOO_CHILD_EVENT == type) {
    LOG_INFO << "zookeeper ZOO_CHILD_EVENT for path: " << path;
    pobj->GetChildrenNodeForPath(path);
  }
}

//同步等待直到获取到数据
int OneZkContainer::GetNNCForPath(const string &path,
                                  ucloud::uns::NameNodeContent &nnc,
                                  int timeout) {
  auto util_time =
      std::chrono::system_clock::now() + std::chrono::milliseconds(timeout);
  while (true) {
    std::unique_lock<std::mutex> lock(
        mutex_);  // 加锁后，后面调用的函数不能再加锁，否则死锁
    if (path_to_nodes_.find(path) ==
        path_to_nodes_.end()) {  // 找不到从来没有加入
      LOG_ERROR << "zookeeper path nerver added, path: " << path;
      return -1;
    }
    auto &cache_state = cache_state_[path];
    if (cache_state.has_data == true) {
      //需要随机选择一个节点
      vector<string> &nodes = path_to_nodes_[path];
      int node_count = nodes.size();
      if (node_count == 0) {  // 可能就没有nodes
        LOG_FATAL << "zookeeper impossible! no node for path: " << path;
      }
      int selected = random() % node_count;
      string &selected_node = nodes[selected];
      string node_path = path + "/" + selected_node;
      auto it = node_to_nnc_.find(node_path);
      if (it == node_to_nnc_.end()) {
        LOG_FATAL << "zookeeper impossible! no nnc for node path: " << node_path
                  << " zk_server: " << zk_server_;
      }
      nnc = it->second;
      if (cache_state.is_dirty == true) {
        LOG_INFO << "zookeeper has no data for path: " << path;
        GetChildrenNodeForPath(path.c_str());
      }
      return 0;
    }
    if (cache_state.error_flag == true) {
      LOG_ERROR << "zookeeper error flag for path:" << path;
      cache_state.error_flag = false;
      break;
    }
    if (timeout == 0) {
      break;
    }
    GetChildrenNodeForPath(path.c_str());
    if (cond_.wait_until(lock, util_time) == std::cv_status::timeout) {
      LOG_ERROR << "zookeeper timeout, path: " << path;
      break;
    }
  }
  return -1;
}

int OneZkContainer::GetNNCForPath(const string &path, const GetNNCCb &cb) {
  ucloud::uns::NameNodeContent nnc;
  std::unique_lock<std::mutex> lock(
      mutex_);  // 加锁后，后面调用的函数不能再加锁，否则死锁
  if (path_to_nodes_.find(path) ==
      path_to_nodes_.end()) {  // 找不到从来没有加入
    LOG_ERROR << "zookeeper path nerver added, path: " << path;
    lock.unlock();
    cb(-1, nnc);
    return -1;
  }
  auto &cache_state = cache_state_[path];
  if (cache_state.has_data == true) {
    //需要随机选择一个节点
    vector<string> &nodes = path_to_nodes_[path];
    int node_count = nodes.size();
    if (node_count == 0) {  // 可能就是没有nodes
      LOG_ERROR << "zookeeper no node for path: " << path;
      GetChildrenNodeForPath(path.c_str());
      lock.unlock();
      cb(-1, nnc);
      return -1;
    }
    int selected = random() % node_count;
    string &selected_node = nodes[selected];
    string node_path = path + "/" + selected_node;
    auto it = node_to_nnc_.find(node_path);
    if (it == node_to_nnc_.end()) {
      LOG_FATAL << "zookeeper impossible! no nnc for node path: " << node_path
                << " zk_server: " << zk_server_;
    }
    if (cache_state.is_dirty == true) {
      LOG_WARN << "use dirty cache for path: " << path;
      GetChildrenNodeForPath(path.c_str());  // dirty状态触发更新
    }
    lock.unlock();
    cb(0, it->second);  // 触发回调
    return 0;
  }
  // 一定是dirty
  if (cache_state.is_dirty == true) {
    if (GetChildrenNodeForPath(path.c_str()) != 0) {
      LOG_ERROR << "zookeeper path:" << path << " get children error";
      lock.unlock();
      cb(-1, nnc);
    } else {
      LOG_INFO << "zookeeper path:" << path << " callback add to waiting list";
      path_cb_list_[path].emplace_back(cb);
    }
  } else {
    LOG_FATAL << "zookeeper cache has no data when cache clean, impossible!";
  }
  return 0;
}

int OneZkContainer::GetNNCForNode(const string &path, const string &node_name,
                                  ucloud::uns::NameNodeContent &nnc,
                                  int timeout) {
  auto util_time =
      std::chrono::system_clock::now() + std::chrono::milliseconds(timeout);
  while (true) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (path_to_nodes_.find(path) == path_to_nodes_.end()) {
      LOG_ERROR << "zookeeper path nerver added, path: " << path;
      return -1;
    }
    auto &cache_state = cache_state_[path];
    if (cache_state.has_data == true) {
      std::vector<string> &nodes = path_to_nodes_[path];
      auto it = std::find(nodes.begin(), nodes.end(), node_name);
      if (it == nodes.end()) {
        LOG_ERROR << "zookeeper node not exist in zk, path_: " << path
                  << " node: " << node_name << " zk_server: " << zk_server_;
        GetChildrenNodeForPath(path.c_str());
        return -1;
      }
      std::string node_path = path + "/" + node_name;
      if (node_to_nnc_.find(node_path) == node_to_nnc_.end()) {
        LOG_ERROR << "zookeeper no nnc for node_path: " << node_path
                  << " zk_server: " << zk_server_;
        GetChildrenNodeForPath(path.c_str());  //触发一次获取该路径下的节点信息
        return -1;
      }
      nnc = node_to_nnc_[node_path];
      if (cache_state.is_dirty == true) {
        LOG_WARN << "zookeeper use dirty cache for path: " << path;
        GetChildrenNodeForPath(path.c_str());  // dirty状态触发更新
      }
      return 0;
    }
    if (cache_state.error_flag == true) {
      LOG_ERROR << "zookeeper error flag for path:" << path;
      cache_state.error_flag = false;
      break;
    }
    if (timeout == 0) {
      break;
    }
    GetChildrenNodeForPath(path.c_str());
    if (cond_.wait_until(lock, util_time) == std::cv_status::timeout) {
      LOG_ERROR << "zookeeper timeout, node" << node_name << " path: " << path;
      break;
    }
  }
  return -1;
}

int OneZkContainer::GetNNCForNode(const string &path, const string &node_name,
                                  const GetNNCCb &cb) {
  ucloud::uns::NameNodeContent nnc;
  std::unique_lock<std::mutex> lock(mutex_);
  if (path_to_nodes_.find(path) == path_to_nodes_.end()) {
    LOG_ERROR << "zookeeper path nerver added, path: " << path;
    lock.unlock();
    cb(-1, nnc);
    return -1;
  }
  auto &cache_state = cache_state_[path];
  if (cache_state.has_data == true) {
    std::vector<string> &nodes = path_to_nodes_[path];
    auto it = std::find(nodes.begin(), nodes.end(), node_name);
    if (it == nodes.end()) {
      LOG_ERROR << "zookeeper node not exist in zk, path_: " << path
                << " node: " << node_name << " zk_server: " << zk_server_;
      GetChildrenNodeForPath(path.c_str());
      lock.unlock();
      cb(-1, nnc);
      return -1;
    }
    std::string node_path = path + "/" + node_name;
    if (node_to_nnc_.find(node_path) == node_to_nnc_.end()) {
      LOG_ERROR << "zookeeper no nnc for node_path: " << node_path
                << " zk_server: " << zk_server_;
      GetChildrenNodeForPath(path.c_str());  //触发一次获取该路径下的节点信息
      lock.unlock();
      cb(-1, nnc);
      return -1;
    }
    nnc = node_to_nnc_[node_path];
    if (cache_state.is_dirty == true) {
      LOG_WARN << "zookeeper use dirty cache for path: " << path;
      GetChildrenNodeForPath(path.c_str());  // dirty状态触发更新
    }
    lock.unlock();
    cb(0, nnc);
    return 0;
  }
  // 一定是dirty
  if (cache_state.is_dirty == true) {
    if (GetChildrenNodeForPath(path.c_str()) != 0) {
      LOG_ERROR << "zookeeper path:" << path << " get children error";
      lock.unlock();
      cb(-1, nnc);
    } else {
      LOG_INFO << "zookeeper path:" << path << " node:" << node_name
               << " callback add to waiting list";
      node_cb_list_[std::make_pair(path, node_name)].emplace_back(cb);
    }
  } else {
    LOG_FATAL << "zookeeper cache has no data when cache clean, impossible!";
  }
  return 0;
}

int OneZkContainer::GetAllNNCForPath(
    const string &path,
    std::map<std::string, ucloud::uns::NameNodeContent> &node_to_nnc,
    int timeout) {
  auto util_time =
      std::chrono::system_clock::now() + std::chrono::milliseconds(timeout);
  while (true) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (path_to_nodes_.find(path) ==
        path_to_nodes_.end()) {  // 找不到说明没有添加过
      LOG_ERROR << "zookeeper path nerver added, path: " << path;
      return -1;
    }
    auto &cache_state = cache_state_[path];
    if (cache_state.has_data == true) {
      vector<string> &nodes = path_to_nodes_[path];
      int node_count = nodes.size();
      if (node_count == 0) {
        LOG_ERROR << "zookeeper impossible! no node for path: " << path;
        return -1;
      }
      string node_path;
      for (auto it = nodes.begin(); it != nodes.end(); it++) {
        node_path = path + "/" + *it;
        if (node_to_nnc_.find(node_path) == node_to_nnc_.end()) {
          LOG_ERROR << "zookeeper no nnc for node: " << node_path;
          GetChildrenNodeForPath(path.c_str());
          return -1;
        }
        node_to_nnc[node_path] = node_to_nnc_[node_path];
      }
      if (cache_state.is_dirty == true) {
        LOG_WARN << "zookeeper use dirty cache for path: " << path;
        GetChildrenNodeForPath(path.c_str());  // dirty状态触发更新
      }
      return 0;
    }
    if (cache_state.error_flag == true) {
      LOG_ERROR << "zookeeper error flag for path:" << path;
      cache_state.error_flag = false;
      break;
    }
    if (timeout == 0) {
      break;
    }
    GetChildrenNodeForPath(path.c_str());
    if (cond_.wait_until(lock, util_time) == std::cv_status::timeout) {
      LOG_ERROR << "zookeeper timeout, path: " << path;
      break;
    }
  }
  return -1;
}

int OneZkContainer::GetAllNNCForPath(const string &path,
                                     const GetAllNNCCb &cb) {
  std::map<std::string, ucloud::uns::NameNodeContent> all_nnc;
  std::unique_lock<std::mutex> lock(mutex_);
  if (path_to_nodes_.find(path) ==
      path_to_nodes_.end()) {  // 找不到说明没有添加过
    LOG_ERROR << "zookeeper path nerver added, path: " << path;
    lock.unlock();
    cb(-1, all_nnc);
    return -1;
  }
  auto &cache_state = cache_state_[path];
  if (cache_state.has_data == true) {
    vector<string> &nodes = path_to_nodes_[path];
    int node_count = nodes.size();
    if (node_count == 0) {
      LOG_ERROR << "zookeeper impossible, no node for path: " << path;
      GetChildrenNodeForPath(path.c_str());  // dirty状态触发更新
      lock.unlock();
      cb(-1, all_nnc);
      return -1;
    }
    string node_path;
    for (auto it = nodes.begin(); it != nodes.end(); it++) {
      node_path = path + "/" + *it;
      if (node_to_nnc_.find(node_path) == node_to_nnc_.end()) {
        LOG_ERROR << "zookeeper no nnc for node: " << node_path;
        GetChildrenNodeForPath(path.c_str());
        lock.unlock();
        cb(-1, all_nnc);
        return -1;
      }
      all_nnc[node_path] = node_to_nnc_[node_path];
    }
    if (cache_state.is_dirty == true) {
      LOG_WARN << "zookeeper use dirty cache for path: " << path;
      GetChildrenNodeForPath(path.c_str());  // dirty状态触发更新
    }
    lock.unlock();
    cb(0, all_nnc);
    return 0;
  }
  // cache中没有数据，一定是dirty
  if (cache_state.is_dirty == true) {
    if (GetChildrenNodeForPath(path.c_str()) != 0) {
      LOG_ERROR << "zookeeper path:" << path << " get children error";
      lock.unlock();
      cb(-1, all_nnc);
    } else {
      LOG_INFO << "zookeeper path:" << path << " callback add to waiting list";
      all_nnc_cb_list_[path].emplace_back(cb);
    }
  } else {
    LOG_FATAL << "zookeeper cache has no data when cache clean, impossible!";
  }
  return 0;
}

int OneZkContainer::Create(const std::string &path, const std::string &data,
                           bool temporary) {
  int flags = temporary ? ZOO_EPHEMERAL : 0;
  int rc = zoo_create(zhandle_, path.c_str(), data.c_str(), data.length(),
                      &ZOO_OPEN_ACL_UNSAFE, flags, nullptr, 0);
  LOG_INFO << "create zookeeper node path: " << path << " error: " << rc;
  return rc;
}

/**
 * @brief 创建ZNode，并且递归创建父Znode
 * @param path 节点路径
 * @param data 节点数据
 * @param temporary 是否是临时节点
 */
int OneZkContainer::CreateRecursive(const std::string &path,
                                    const std::string &data, bool temporary) {
  int rc;
  size_t pos = path.find('/', 1);
  while (pos != std::string::npos) {
    //递归创建父ZNode
    LOG_INFO << "zookeeper create path: " << path.substr(0, pos)
             << " pos: " << pos;
    rc = zoo_create(zhandle_, path.substr(0, pos).c_str(), NULL, -1,
                    &ZOO_OPEN_ACL_UNSAFE, 0, nullptr, 0);
    if (rc != ZOK && rc != ZNODEEXISTS) {
      LOG_ERROR << "zookeeper create recursive zookeeper node error, path: "
                << path.substr(0, pos) << " error: " << rc;
      return rc;
    }
    pos = path.find('/', pos + 1);
  }
  return Create(path, data, temporary);
}

// 业务和过期回调中都可能调用
int OneZkContainer::RegisterInstance(const std::string &path,
                                     const std::string &data) {
  LOG_INFO << "zookeeper will register instance path: " << path
           << ", data: " << data;
  if (zhandle_ == NULL) {
    LOG_WARN << "zookeeper handle is null when RegisterInstance";
    ConnectToServer();
  }
  std::unique_lock<std::mutex> lock(mutex_);
  int rc;
  do {
    //已被成功注册，则直接返回
    auto it = instance_registered_.find(path);
    if (it != instance_registered_.end() && it->second) {
      LOG_INFO
      << "zookeeper instance has registered successfully! Notify Caller";
      return ZOK;
    }
    rc = CreateRecursive(path, data, true);
    if (rc == ZOK) {
      LOG_INFO << "register instance: " << path << " successfully";
      break;
    } else if (rc == ZNODEEXISTS) {
      LOG_INFO << "zookeeper this path is already exist, this will try again "
                  "after 1s";
    } else {
      LOG_SYSFATAL << "rc: " << rc << "register instance error, " << zerror(rc);
    }
    sleep(1);
  } while (rc != ZOK);
  // 注册成功，置标志为true
  instance_registered_[path] = true;
  return rc;
}

int OneZkContainer::UnRegisterInstance(const std::string &path) {
  LOG_INFO << "zookeeper will unregister instance path: " << path;
  std::unique_lock<std::mutex> lock(mutex_);
  int version = -1;
  int rc = zoo_delete(zhandle_, path.c_str(), version);
  if ((ZOK == rc) || (ZNONODE == rc)) {
    auto it = instance_registered_.find(path);
    if (it != instance_registered_.end()) {
      instance_registered_.erase(it);
    }
    return ZOK;
  }
  LOG_ERROR << "zookeeper rc: " << rc << " unregister instance error, "
            << zerror(rc);
  return -1;
}

/**
 * @brief 注册leader节点, 业务和过期回调中都可能调用
 * @param path 节点路径
 * @param data 节点数据
 * @param cb watch回调函数
 * @param ctx watch上下文
 * @return ZOK表示leader创建成功 ZNODEEXISTS表示watch leader成功
 */
int OneZkContainer::RegisterLeader(const std::string &path,
                                   const std::string &data,
                                   const WatchLeaderCb &cb, void *arg) {
  LOG_INFO << "zookeeper will register leader path: " << path
           << ", data: " << data;
  if (zhandle_ == NULL) {
    LOG_WARN << "zookeeper handle is null when RegisterLeader";
    ConnectToServer();
  }
  std::unique_lock<std::mutex> lock(mutex_);
  int rc;
  int watch_ret = ZOK;
  do {
    //已被成功注册，则直接返回
    auto it = instance_registered_.find(path);
    if (it != instance_registered_.end() && it->second) {
      LOG_INFO
      << "zookeeper leader has registered successfully! Notify Caller";
      return ZOK;
    }
    rc = CreateRecursive(path, data, true);
    if (rc == ZOK) {
      LOG_INFO << "zookeeper register leader success, I am leader";
      instance_registered_[path] = true;
      break;
    } else if (rc == ZNODEEXISTS) {
      watch_ret = WatchLeader(path, data, cb, arg);
      if (watch_ret != ZOK) {
        LOG_ERROR << "zookeeper watch_ret: " << watch_ret
                  << "watch leader error, " << zerror(watch_ret);
      } else {
        LOG_INFO << "zookeeper watch leader success.";
        break;
      }
    } else {
      LOG_SYSFATAL << "zookeeper rc: " << rc << "register leader error, "
                   << zerror(rc);
    }
    sleep(1);
  } while ((rc != ZOK && rc != ZNODEEXISTS) || watch_ret != ZOK);
  return rc;
}

int OneZkContainer::WatchLeader(const std::string &path,
                                const std::string &data,
                                const WatchLeaderCb &cb, void *arg) {
  char buffer[2048];
  int buffer_len = 2048;
  WatchEntry *watch_entry = new WatchEntry(path, data, this, cb, arg);
  int ret = zoo_wget(zhandle_, path.c_str(), WatchLeaderHandler,
                     (void *)watch_entry, buffer, &buffer_len, nullptr);
  if (ret == ZOK) {  // 已经有leader
    cb(false, arg);
  } else {  // get时获取数据失败，返回错误，重新注册leader
    delete watch_entry;
    LOG_ERROR << "zookeeper ret: " << ret << "watch leader error:, "
              << zerror(ret);
  }
  return ret;
}

void OneZkContainer::WatchLeaderHandler(zhandle_t *zh, int type, int state,
                                        const char *path, void *watcher_ctx) {
  // session事件由session_watcher处理
  int ret = 0;
  if (type == ZOO_SESSION_EVENT) {
    return;
  }
  WatchEntry *watch_entry = (WatchEntry *)watcher_ctx;
  OneZkContainer *pobj = reinterpret_cast<OneZkContainer *>(watch_entry->ctx);
  // 节点删除，注册
  if (type == ZOO_DELETED_EVENT) {
    LOG_INFO << "zookeeper leader deleted";
    {
      std::unique_lock<std::mutex> lock(pobj->mutex_);
      pobj->instance_registered_[path] = false;
    }
    ret = pobj->RegisterLeader(watch_entry->path, watch_entry->data,
                               watch_entry->cb, watch_entry->arg);
    bool is_leader = (ret == ZOK) ? true : false;
    watch_entry->cb(is_leader, watch_entry->arg);
    delete watch_entry;
    return;
  } else {
    if (type == ZOO_CHANGED_EVENT) {
      char buffer[2048];
      int buffer_len = 2048;
      // 节点数据变化，重新获取数据
      ret = zoo_wget(zh, path, WatchLeaderHandler, (void *)watch_entry, buffer,
                     &buffer_len, nullptr);
      if (ret == ZOK) {
        LOG_INFO << "zookeeper get leader path: " << path
                 << ", data: " << std::string(buffer, buffer_len);
        watch_entry->cb(false, watch_entry->arg);
        return;
      }
    } else if (type == ZOO_NOTWATCHING_EVENT) {
      // nerver!
    }
    LOG_SYSFATAL << "zookeeper watch leader error: " << ret
                 << ", type: " << type;
  }
}

TimerId OneZkContainer::reset_reconnect_timer() {
  auto empty_timer_id = uevent::TimerId();
  if (reconnect_timer_id_ == empty_timer_id) {
    return reconnect_timer_id_;
  }
  auto tmp = reconnect_timer_id_;
  reconnect_timer_id_ = TimerId();
  return tmp;
}

bool OneZkContainer::set_reconnect_timer(TimerId timer_id) {
  auto empty_timer_id = uevent::TimerId();
  if (reconnect_timer_id_ != empty_timer_id) {
    return false;
  }
  reconnect_timer_id_ = timer_id;
  return true;
}

}  // namespace uevent
