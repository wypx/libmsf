#ifndef USTEVENT_ONE_ZK_CONTAINER_H_
#define USTEVENT_ONE_ZK_CONTAINER_H_

#include <zookeeper/zookeeper.h>
#include <condition_variable>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <functional>
#include "umessage.h"
#include "uns.55000.56000.pb.h"
#include "../timer.h"
#include "../timer_id.h"

using namespace MSF;

namespace MSF {

typedef std::function<void(bool is_leader, void *arg)> WatchLeaderCb;
typedef std::function<void()> SessionStateChangeCb;
typedef std::function<void(int, ucloud::uns::NameNodeContent)> GetNNCCb;
typedef std::function<
    void(int, std::map<std::string, ucloud::uns::NameNodeContent>)> GetAllNNCCb;

class OneZkContainer {
 public:
  struct GetChildrenCtx {
    void *ctx;
    std::string path;
  };
  struct GetNodeDataCtx {
    void *ctx;
    std::string node;
  };

  struct CacheState {
    CacheState() : has_data(false), is_dirty(true), error_flag(false) {}
    bool has_data;
    bool is_dirty;
    //遇到错误终止同步等待
    bool error_flag;
  };

  explicit OneZkContainer(const std::string &zk_server, int timeout);
  ~OneZkContainer();

  void AddZkPath(const std::string &zk_path);
  int Init();
  bool init_ready();
  int GetChildrenNode();
  int GetChildrenNodeForPath(const char *path);
  int GetDataForNode(const char *node);

  void CallCb(int ret, const std::string &path,
              std::unique_lock<std::mutex> &lock);
  int GetNNCForPath(const string &path, ucloud::uns::NameNodeContent &nnc,
                    int timeout);  // timeout 毫秒
  int GetNNCForPath(const string &path, const GetNNCCb &cb);

  int GetNNCForNode(const string &path, const string &node_name,
                    ucloud::uns::NameNodeContent &nnc,
                    int timeout);  // timeout 毫秒
  int GetNNCForNode(const string &path, const string &node_name,
                    const GetNNCCb &cb);

  int GetAllNNCForPath(
      const string &path,
      std::map<std::string, ucloud::uns::NameNodeContent> &node_to_nnc,
      int timeout);  // timeout 毫秒
  int GetAllNNCForPath(const string &path, const GetAllNNCCb &cb);

  void set_session_connected_cb(const SessionStateChangeCb &cb) {
    session_connected_cb_ = cb;
  }
  void set_session_connecting_cb(const SessionStateChangeCb &cb) {
    session_connecting_cb_ = cb;
  }
  void set_session_expired_cb(const SessionStateChangeCb &cb) {
    session_expired_cb_ = cb;
  }
  void set_session_expired_2_connected_cb(const SessionStateChangeCb &cb) {
    session_expired_2_connected_cb_ = cb;
  }

  int RegisterInstance(const std::string &path, const std::string &data);
  int UnRegisterInstance(const std::string &path);

  int RegisterLeader(const std::string &path, const std::string &data,
                     const WatchLeaderCb &cb, void *arg);
  int ConnectToServer();
  TimerId reset_reconnect_timer();
  bool set_reconnect_timer(TimerId timer_id);
  void reset_status();

 private:
  int ConnectToServerInternal();
  void SetAllNodeDirty();
  void NotifyZkInitReady();
  bool WaitZkInitReady();

  static void ZkInitWatcher(zhandle_t *zh, int type, int state,
                            const char *path, void *watcherCtx);
  static void NodeDataWatcher(zhandle_t *zh, int type, int state,
                              const char *path, void *watcherCtx);
  static void DataCompleteCb(int rc, const char *value, int value_len,
                             const struct Stat *stat, const void *data);
  static void ChildrenWatcher(zhandle_t *zh, int type, int state,
                              const char *path, void *watcherCtx);
  static void GetChildenCompleteCb(int rc, const struct String_vector *str_vec,
                                   const void *data);

  int WatchLeader(const std::string &path, const std::string &data,
                  const WatchLeaderCb &cb, void *ctx);
  static void WatchLeaderHandler(zhandle_t *zh, int type, int state,
                                 const char *path, void *watcher_ctx);

  int Create(const std::string &path, const std::string &data, bool temporary);
  int CreateRecursive(const std::string &path, const std::string &data,
                      bool temporary);

 private:
  std::string zk_server_;
  int timeout_;
  zhandle_t *zhandle_;

  int last_session_state_ = ZOO_EXPIRED_SESSION_STATE;
  // session expired 时触发该回调
  SessionStateChangeCb session_expired_cb_;
  // 状态从expired变成connected时触发该回调
  SessionStateChangeCb session_expired_2_connected_cb_;
  // 状态从connecting变成connected时触发该回调
  SessionStateChangeCb session_connected_cb_;
  // 状态变成connecting时触发该回调, 之前状态可能是expired或者connectd
  SessionStateChangeCb session_connecting_cb_;

  mutable std::mutex mutex_;  // 防止业务线程和zookeeper线程同时修改共享的数据
  mutable std::mutex connectionMutex_;  // 连接锁，防止多个zk连接
  std::condition_variable cond_;
  // name路径到node 的映射 一对多
  std::map<std::string, std::vector<std::string>> path_to_nodes_;
  //节点 node 到 NNC 的映射 一对一
  std::map<std::string, ucloud::uns::NameNodeContent> node_to_nnc_;
  std::atomic<bool> init_ready_;

  std::map<std::string, CacheState> cache_state_;

  // 保证path下所有node都成功拉到信息后才将cache_has_data更新为true
  struct GetNodeDataState {
    bool all_node_success = true;
    int inflight_node_count = 0;
  };
  std::map<std::string, GetNodeDataState> path_to_get_node_data_state_;

  // 记录需要等待执行的回调
  std::map<std::string, std::vector<GetNNCCb>> path_cb_list_;
  std::map<std::pair<std::string, std::string>, std::vector<GetNNCCb>>
      node_cb_list_;
  std::map<std::string, std::vector<GetAllNNCCb>> all_nnc_cb_list_;

  // 防止多个线程同时注册导致，循环循环注册
  std::map<std::string, bool> instance_registered_;
  TimerId reconnect_timer_id_;
};

}  // namespace MSF

#endif
