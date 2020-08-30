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
#include <Zookeeper/Zookeeper.h>

using namespace MSF;

namespace MSF {

// 127.0.0.1:2181,127.0.0.1:2182,127.0.0.1:2183
const char UZookeeperBase::hostPorts[] = "127.0.0.1:2181";

UZookeeperBase::UZookeeperBase() {
  pstZkHandle = NULL;
  iConnectTimeout = 1500;
  iMaxReConnectCnt = 10;
  pstZkBase = this;
  iConnected = false;
}

UZookeeperBase::~UZookeeperBase() {
  if (pstZkHandle != NULL) {
    zookeeper_close(pstZkHandle);
    pstZkHandle = NULL;
  }
}

UZookeeperBase* UZookeeperBase::pstZkBase = NULL;

void UZookeeperBase::InitWatcher(zhandle_t* zh, int type, int state,
                                 const char* path, void* watcher_ctx) {
  if (state == ZOO_CONNECTED_STATE) {
    pstZkBase->iConnected = true;
    cout << "InitWatcher() ZOO_CONNECTED_STATE" << endl;
  } else if (state == ZOO_NOTCONNECTED_STATE) {
    pstZkBase->iConnected = false;
    cout << "InitWatcher() ZOO_NOTCONNECTED_STATE" << endl;
  } else if (state == ZOO_AUTH_FAILED_STATE) {
    cout << "InitWatcher() ZOO_AUTH_FAILED_STATE" << endl;
  } else if (state == ZOO_EXPIRED_SESSION_STATE) {
    //在接收到ZOO_EXPIRED_SESSION_STATE事件后设置为过期状态（并关闭会话句柄）
    pstZkBase->iConnected = false;
    zookeeper_close(pstZkBase->pstZkHandle);
    pstZkBase->pstZkHandle = NULL;
    cout << "InitWatcher() ZOO_EXPIRED_SESSION_STATE" << endl;
    pstZkBase->ConnectZookeeper();
  } else if (state == ZOO_CONNECTING_STATE) {
    cout << "InitWatcher() ZOO_CONNECTING_STATE" << endl;
  } else if (state == ZOO_ASSOCIATING_STATE) {
    cout << "InitWatcher() ZOO_ASSOCIATING_STATE" << endl;
  }

  if (type == ZOO_CREATED_EVENT) {
    cout << "InitWatcher() ZOO_CREATED_EVENT" << endl;
  } else if (type == ZOO_DELETED_EVENT) {
    cout << "InitWatcher() ZOO_DELETED_EVENT" << endl;
  } else if (type == ZOO_CHANGED_EVENT) {
    cout << "InitWatcher() ZOO_CHANGED_EVENT" << endl;
  } else if (type == ZOO_CHILD_EVENT) {
    cout << "InitWatcher() ZOO_CHILD_EVENT" << endl;
  } else if (type == ZOO_SESSION_EVENT) {
    cout << "InitWatcher() ZOO_SESSION_EVENT" << endl;
  }
}

/* 步骤1: 开始会话
 * ZooKeeper进行任何操作之前，我们首先需要一个zhandle_t句柄
 * */
int UZookeeperBase::ConnectZookeeper() {
  cout << "Now try to connect zk host:" << getHostPorts() << endl;

  int iConnTimes = 0;
  if (pstZkHandle != NULL) {
    zookeeper_close(pstZkHandle);
  }

  zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);  //设置log日志的输出级别
  do {
    pstZkHandle = zookeeper_init(getHostPorts(), InitWatcher, iConnectTimeout,
                                 NULL, NULL, 0);
  } while (!iConnected && (iConnTimes++) < iMaxReConnectCnt);

#if 0
    for (int i = 0; i < pstZkHandle->addrs.count; i++){
        sockaddr_in* addr=(struct sockaddr_in*)&pstZkHandle->addrs.data[i];
    }
#endif

  if (iConnTimes >= iMaxReConnectCnt) {
    cout << "Connect zk host " << getHostPorts()
         << " over max times:" << iConnTimes << endl;
    return -1;
  }

  return 0;
}

//服务端
UZookeeperSrv* UZookeeperSrv::pstZkSrv = NULL;

UZookeeperSrv::UZookeeperSrv(/* args */) { pstZkSrv = this; }

void UZookeeperSrv::CreateParentComplete(int rc, const char* value,
                                         const void* data) {
  cout << "CreateParentComplete --> zoo_create() path=" << value << ","
       << zerror(rc) << endl;
  switch (rc) {
    //判断返回码来确定需要如何处理
    case ZCONNECTIONLOSS:
      cout << "CreateParentComplete: Try again later." << endl;
      pstZkSrv->CreateParent(value,
                             (const char*)data);  //在连接丢失时进行重试操作。
      break;
    case ZOK:
      cout << "CreateParentComplete: Created parent node successful" << endl;
      break;
    case ZNODEEXISTS:
      cout << "CreateParentComplete: Node already exists." << endl;
      break;
    default:
      cout << "CreateParentComplete: Something went wrong when running for "
              "master."
           << endl;
      break;
  }
}

/* *
 * zoo_acreate 为异步调用系列
 * zoo_create 为同步调用
 * 参数1: zkhandle
 * 参数2: 该方法的path参数类型为const
 * char*类型,path用于将客户端与对应的znode节点的子树连接在一起 参数3:
 * 该函数的第三个参数为存储到znode节点的数据信息 参数4:
 * 该参数为保存数据信息（前一个参数）的长度值,本例中设置为0 参数5:
 * 本例中，我们并不关心ACL策略问题,所以我们均设置为unsafe模式 参数6:
 * 根结点些znode节点为持久性的非有序节点,所以我们不需要传入任何标志位 参数7:
 * 异步调用,我们需要传入一个完成函数,ZooKeeper客户端会在操作请求完成时调用该函数
 *        同步调用,
 * 参数8: 异步调用，最后一个参数为上下文变量, 本例中不需要传入任何上下文变量
 *        同步调用,
 * */
void UZookeeperSrv::CreateZnode(const char* path, const char* value,
                                int valuelen, int flags,
                                string_completion_t completion) {
  zoo_acreate(pstZkHandle, path, value, valuelen, &ZOO_OPEN_ACL_UNSAFE, flags,
              completion, NULL);
}

void UZookeeperSrv::CreateParent(const char* path, const char* value) {
  CreateZnode(path, value, 0, 0, CreateParentComplete);
}

/* 步骤2: 引导主节点
 * 引导（Bootstraping）主节点是指创建主从模式例子中使用的一些znode节点并竞选主要主节点的过程。
 * 我们首先创建四个必需的znode节点：
 * 创建四个父节点：/workers、/assign、/tasks、/status
 * */
void UZookeeperSrv::Bootstrap() {
  if (!iConnected) {
    cout << "Client not connected to ZooKeeper." << endl;
    return;
  }
  CreateParent("/workers", "");
  CreateParent("/assign", "");
  CreateParent("/tasks", "");
  CreateParent("/status", "");
}

//在znode节点被删除时，我们会收到通知消息
void UZookeeperSrv::MasterExistWatcher(zhandle_t* zh, int type, int state,
                                       const char* path, void* watcherCtx) {
  if (type == ZOO_DELETED_EVENT) {
    if (!strcmp(path, "/master")) {
      //严重错误 assert
    }
    //如果/master节点被删除，开始竞选主节点流程。
    pstZkSrv->CreateMaster();
  } else {
    cout << "Watched event:" << type << endl;
  }
}

void UZookeeperSrv::MasterExistComplete(int rc, const struct Stat* stat,
                                        const void* data) {
  switch (rc) {
    case ZCONNECTIONLOSS:
    case ZOPERATIONTIMEOUT:
      pstZkSrv->MasterExist();
      break;
    case ZOK:
      if (stat == NULL) {
        //通过判断返回的stat是否为null来检查该znode节点是否存在。
        cout << "Previous master is gone, running for master." << endl;
        pstZkSrv->CreateMaster();  //如果节点不存在，再次进行主节点竞选的流程。
      }
      break;
    default:
      cout << "Something went wrong when executing exists:" << rc << endl;
      break;
  }
}

void UZookeeperSrv::MasterExist() {
  zoo_awexists(pstZkHandle, "/master",
               MasterExistWatcher,  //定义/master节点的监视点。
               NULL,
               MasterExistComplete,  //该exists函数的回调方法的参数。
               NULL);
}

void UZookeeperSrv::CreateMasterComplete(int rc, const char* value,
                                         const void* data) {
  switch (rc) {
    case ZCONNECTIONLOSS:
      // check_master();//连接丢失时，检查主节点的znode节点是否已经创建成功，或被其他主节点进程创建成功。
      break;
    case ZOK:
      //如果我们成功创建了节点，就获得了管理权资格
      pstZkSrv->MakeLeaderShip();
      break;
    case ZNODEEXISTS:
      //如果该主节点进程发现/master节点已经存在,
      //就需要通过zoo_awexists方法来设置一个监视点
      //如果主节点znode节点存在(其他进程已经创建并锁定该节点),
      //就对该节点建立监视点, 来监视该节点之后可能消失的情况。
      pstZkSrv->MasterExist();
      break;
    default:
      cout << "CreateMasterComplete: Something went wrong when running for "
              "master."
           << endl;
      break;
  }
}

//下一个任务为竞选主节点,竞选主节点主要就是尝试创建/master节点,以便锁定主要主节点角色
void UZookeeperSrv::CreateMaster() {
  if (!iConnected) {
    cout << "Client not connected to ZooKeeper." << endl;
    return;
  }
  int server_id = 1;
  char server_id_string[9];
  snprintf(server_id_string, 9, "%x", server_id);
  CreateZnode("/master", (const char*)server_id_string, sizeof(int),
              ZOO_EPHEMERAL, CreateMasterComplete);
}

void UZookeeperSrv::MakeLeaderShip() { GetWorkers(); }
void UZookeeperSrv::GetWorkers() {}

void UZookeeperSrv::GetWorkersComplete(int rc,
                                       const struct String_vector* strings,
                                       const void* data) {}

void UZookeeperSrv::CreateUDiskIpGroupComplete(int rc, const char* value,
                                               const void* data) {
  switch (rc) {
    //判断返回码来确定需要如何处理
    case ZCONNECTIONLOSS:
      cout << "CreateUDiskIpGroupComplete: Try it again later." << endl;
      pstZkSrv->CreateUDiskIpGroup();  //在连接丢失时进行重试操作。
      break;
    case ZOK:
    case ZNODEEXISTS:
      cout << "CreateUDiskIpGroupComplete:" << zerror(rc) << endl;
      pstZkSrv->CreateUDiskIpItem();  //创建子节点上线
      break;
    default:
      cout << "CreateUDiskIpGroupComplete: Something wrong with master, "
           << zerror(rc) << "." << endl;
      break;
  }
}

void UZookeeperSrv::CreateUDiskIpGroup() {
  if (!iConnected) {
    cout << "CreateUDiskIpGroup: Client not connected to ZooKeeper." << endl;
    return;
  }
  CreateZnode("/udisk_ip", "", 0, 0, CreateUDiskIpGroupComplete);
}

void UZookeeperSrv::CreateUDiskIpItemComplete(int rc, const char* value,
                                              const void* data) {
  cout << "CreateUDiskIpItemComplete --> zoo_create() path=" << value
       << ",ret:" << zerror(rc) << endl;
  switch (rc) {
    //判断返回码来确定需要如何处理
    case ZCONNECTIONLOSS:
      cout << "CreateUDiskIpItemComplete: Try again later." << endl;
      pstZkSrv->CreateParent(value,
                             (const char*)data);  //在连接丢失时进行重试操作。
      break;
    case ZOK:
      cout << "CreateUDiskIpItemComplete: Created ip node successful" << endl;
      break;
    case ZNODEEXISTS:
      cout << "CreateUDiskIpItemComplete: Something wrong with master, "
           << zerror(rc) << "." << endl;
      break;
    default:
      cout << "CreateUDiskIpItemComplete: Something went wrong when running "
              "for master."
           << endl;
      break;
  }
}
void UZookeeperSrv::CreateUDiskIpItem() {
  string pstUDiskIpItemPath = "/udisk_ip/127.0.0.1:8888";
  CreateZnode(pstUDiskIpItemPath.c_str(), pstUDiskIpItemPath.c_str(),
              pstUDiskIpItemPath.length(), ZOO_EPHEMERAL,
              CreateUDiskIpItemComplete);
}

//客户端
//静态变量的初始化
UZookeeperCli* UZookeeperCli::pstZkCli = NULL;

UZookeeperCli::UZookeeperCli(/* args */) { pstZkCli = this; }

void UZookeeperCli::UpdateUDiskIpGroupWatcher(zhandle_t* zh, int type,
                                              int state, const char* path,
                                              void* watcherCtx) {
  cout << "######################################" << endl;
  cout << "type:" << type << endl;
  cout << "state:" << state << endl;
  cout << "path:" << path << endl;
  cout << "ZOO_CHILD_EVENT:" << ZOO_CHILD_EVENT << endl;
  if (ZOO_CHILD_EVENT == type) {
    cout << "ServiceWatcher ZOO_CHILD_EVENT" << endl;
    pstZkCli->UpdateUDiskIpGroup();  //更新服务列表
  }
  cout << "######################################" << endl;
}

void UZookeeperCli::UpdateUDiskIpGroup(/* args */) {
  cout << "UZookeeperCli::Enter UpdateUDiskIpGroup" << endl;
  if (!iConnected) {
    cout << "UpdateUDiskIpGroup: Client not connected to ZooKeeper." << endl;
    return;
  }
  string pstUDiskIpGroup = "/udisk_ip";
  //获得服务份数
  struct String_vector str_vec;

  int iRet = zoo_wget_children(pstZkHandle, pstUDiskIpGroup.c_str(),
                               UpdateUDiskIpGroupWatcher, NULL, &str_vec);
  if (iRet) {
    cout << "UpdateUDiskIpGroup --> read path:" << pstUDiskIpGroup << " wrong, "
         << zerror(iRet) << endl;
    return;
  }

  cout << "UpdateUDiskIpGroup --> path:" << pstUDiskIpGroup << ", ret:" << iRet
       << ", node's size:" << str_vec.count << endl;

  //获得各份服务ip:port
  for (int i = 0; i < str_vec.count; ++i) {
    struct String_vector node_vec;
    string path = pstUDiskIpGroup + "/" + str_vec.data[i];
    iRet = zoo_wget_children(pstZkHandle, path.c_str(),
                             UpdateUDiskIpGroupWatcher, NULL, &node_vec);
    cout << "UpdateUDiskIpGroup --> path:" << path << ", ret:" << iRet
         << ", node's size:" << node_vec.count << endl;

    // https://www.cnblogs.com/lanjianhappy/p/7171341.html
    // /udisk_ip/127.0.0.1:8888

    //正则表达式匹配
    regex pattern("/udisk_ip/(.*):(.*)");
    smatch results;

    int uIdx = 0;
    if (regex_match(path, results, pattern)) {
      for (auto it = results.begin(); it != results.end(); ++it) {
        if (uIdx == 1) {
          pstHost = string(*it);
        } else if (uIdx == 2) {
          pstPort = string(*it);
        }
        cout << uIdx << "====>" << *it << endl;
        uIdx++;
      }
    }

    cout << pstHost << ":" << pstPort << endl;

    if (iRet || node_vec.count != 1) {
      continue;
    }
  }
  //
}

}  // namespace MSF