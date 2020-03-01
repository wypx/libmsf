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
#ifndef __MSF_ZOOKEEPER_H__
#define __MSF_ZOOKEEPER_H__

namespace MSF {
namespace ZOOKEEPER {

#include <unistd.h>

#include <iostream>
#include <regex>
#include <string>

#include "zookeeper.h"
using namespace std;

class UZookeeperBase {
 private:
 protected:
  static const char hostPorts[];
  const char* getHostPorts() { return hostPorts; }
  int iMaxReConnectCnt;  //重连超时次数
  int iConnectTimeout;   //重连超时事件
  int iConnected;
  zhandle_t* pstZkHandle;

 public:
  UZookeeperBase(/* args */);
  ~UZookeeperBase();
  int ConnectZookeeper();
  static UZookeeperBase* pstZkBase;
  static void InitWatcher(zhandle_t* zh, int type, int state, const char* path,
                          void* watcher_ctx);
};

class UZookeeperSrv : public UZookeeperBase {
 private:
  /* data */
 public:
  static UZookeeperSrv* pstZkSrv;
  UZookeeperSrv(/* args */);
  void CreateZnode(const char* path, const char* value, int valuelen, int flags,
                   string_completion_t completion);
  void CreateParent(const char* path, const char* value);
  static void CreateParentComplete(int rc, const char* value, const void* data);
  void Bootstrap();

  static void CreateMasterComplete(int rc, const char* value, const void* data);
  void CreateMaster();
  void MasterExist();
  static void MasterExistWatcher(zhandle_t* zh, int type, int state,
                                 const char* path, void* watcherCtx);
  static void MasterExistComplete(int rc, const struct Stat* stat,
                                  const void* data);

  //行使管理权
  //一旦主节点进程被选定为主要主节点,它就可以开始行使其角色,首先它将获取所有可用从节点的列表信息
  void MakeLeaderShip();
  void GetWorkers();
  static void GetWorkersComplete(int rc, const struct String_vector* strings,
                                 const void* data);

  //测试获取IP和端口
  void CreateUDiskIpGroup();
  static void CreateUDiskIpGroupComplete(int rc, const char* value,
                                         const void* data);
  void CreateUDiskIpItem();
  static void CreateUDiskIpItemComplete(int rc, const char* value,
                                        const void* data);
};

class UZookeeperCli : public UZookeeperBase {
 private:
 public:
  UZookeeperCli();
  //监控服务变化
  static UZookeeperCli* pstZkCli;
  string pstHost;
  string pstPort;
  //更新服务列表，冷备和热备
  void UpdateUDiskIpGroup();
  static void UpdateUDiskIpGroupWatcher(zhandle_t* zh, int type, int state,
                                        const char* path, void* watcherCtx);
};

}  // namespace ZOOKEEPER
}  // namespace MSF

#endif