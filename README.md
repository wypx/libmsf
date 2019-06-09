# **Linux平台微服务框架-libmsf**

[![Build Status](https://travis-ci.org/alibaba/fastjson.svg?branch=master)](https://travis-ci.org/alibaba/fastjson)
[![Codecov](https://codecov.io/gh/alibaba/fastjson/branch/master/graph/badge.svg)](https://codecov.io/gh/alibaba/fastjson/branch/master)
[![Maven Central](https://maven-badges.herokuapp.com/maven-central/com.alibaba/fastjson/badge.svg)](https://maven-badges.herokuapp.com/maven-central/com.alibaba/fastjson/)
[![GitHub release](https://img.shields.io/github/release/alibaba/fastjson.svg)](https://github.com/alibaba/fastjson/releases)
[![License](https://img.shields.io/badge/license-Apache%202-4EB1BA.svg)](https://www.apache.org/licenses/LICENSE-2.0.html)

MSF (Mircro Service Framework) acting as mircro service process framework, provide base infrastructure such as network, epoll event, timer, general logger, mempool and process service monitoring etc.

### __主要特性包括:__
 * 为各个模块提供基础设施模块 比如`内存` `日志` `网络` `事件` `信号`等基础库
 * 提供微服务`进程动态拉起` `动态监控`框架，实现微服务进程快速秒级拉起
 * 提供微服务`so动态加载` `动态卸载`做到插件化的即插即用
 * 提供各个微服务进程的`内存监控`等亚健康状态监控，达到实时告警通知
 * 提供微服务配置动态解析，热切换微服务进程，实现平滑升级过度

![libmsf](http://luotang.me/wp-content/uploads/2018/02/cpluff_architecture.png "libmsf")

## __快速开始__
### 安装环境
```xml
安装Ubuntu, Debian等Linux, ARM环境
测试环境（Linux KaliCI 4.19.0-kali3-amd64）: gcc version 8.2.0 (Debian 8.2.0-14)
测试环境（Linux raspberrypi 4.14.52-v7+）：gcc version 6.4.1 20171012 (Linaro GCC 6.4-2017.11)
```
### 下载开源库

- [微服务框架库][1]
``` groovy
git clone https://github.com/wypx/libmsf/
```
- [微服务通信框库][2]
``` groovy
git clone https://github.com/wypx/librpc/
```
[1]: https://github.com/wypx/libmsf/
[2]: https://github.com/wypx/librpc/


Please see this [Wiki Download Page][Wiki] for more repository infos.

[Wiki]: https://github.com/wypx/libmsf

### 编译开源库
```xml
root@KaliCI:/media/psf/tomato/mod/libmsf make
Make Subdirs msf
make[1]: Entering directory '/media/psf/tomato/mod/libmsf/msf
arm-linux-gnueabihf-gcc lib/src/msf_log.o
.................
make[1]: Leaving directory '/media/psf/tomato/mod/libmsf/msf_daemon

root@KaliCI:/media/psf/tomato/mod/librpc/server make
arm-linux-gnueabihf-gcc bin/src/conn.o
arm-linux-gnueabihf-gcc bin/src/config.o
.......
```

```xml
root@KaliCI:/media/psf/tomato/mod/librpc/client# ls ../../../packet/binary/
msf_agent  msf_daemon  msf_dlna  msf_shell  msf_upnp

root@KaliCI:/media/psf/tomato/mod/librpc/client# ls ../../../packet/library/
libipc.so  libmsf.so
```

```xml
msf_agent  各个服务进程之间的通信代理服务端程序
msf_dlna   测试程序 - 独立微服务进程客户端DLNA
msf_upnp   测试程序 - 独立微服务进程客户端UPNP
msf_daemon 用于守护监控代理进程msf_agent
msf_shell  壳子程序，用于记载配置文件中的插件

libipc.so 提供给各个微服务进程连接的通信代理客户端库
libipc.so 提供给各个微服务进程的基础设施库
          包括：网络 管道 Epoll等事件驱动 日志 共享内存 内存池 
          串口通信 线程 进程 CPU工具 文件 加密 微服务框架 定时器
```
### 运行开源库
```xml
1. 执行样例程序
   $ ./msf_agent
   $ ./msf_upnp
   $ ./msf_dlna
2. 查看运行日志
   运行结果
```

### API使用解析
```
微服务API使用：(动态加载SO的方式, 一个服务就是一个独立的SO)
https://github.com/wypx/libmsf/blob/master/msf/src/msf_svc.c
https://github.com/wypx/libmsf/blob/master/msf_shell/src/msf_shell.c

s32 service_init(void) {
    u32 svc_idx;
    struct msf_svc *svc_cb;
    for (svc_idx = 0; svc_idx < g_proc->proc_svc_num; svc_idx++) {
        if (msf_svcinst_init(&(g_proc->proc_svcs[svc_idx])) < 0) {
            return -1;
        };
    }

    for (svc_idx = 0; svc_idx < g_proc->proc_svc_num; svc_idx++) {
        svc_cb = g_proc->proc_svcs[svc_idx].svc_cb;
        if (svc_cb->init(NULL, 0) < 0) {
            return -1;
        }
    }
    return 0;
}

使用配置文件：
配置文件名字：msf_agent_svc.json
配置文件内容：
{
    "proc_name": "msf_agent",
    "proc_author": "http://luotang.me",
    "proc_version": "beta v1.0",
    "proc_desc": "Micro agent service config file",
    "proc_svcs":
    [
        {
          "svc_name": "msf_agent1",
          "svc_libs": "libmsf_agent1.so"
        },
        {
          "svc_name": "msf_agent2",
          "svc_libs": "libmsf_agent2.so"
        }
    ]
}
配置特点：
支持一个壳子进程拉起多个微服务
支持配置微服务进程的名称，作者，版本，描述，微服务实例信息

使用壳子进程拉起微服务SO：
./msf_shell -c msf_agent_svc.json
```

```
微服务API使用：(进程内静态微服务模块, 类似Nginx的模块加载)
https://github.com/wypx/mobile/blob/master/src/at_mod.c
https://github.com/wypx/mobile/blob/master/src/mobile.c

extern struct msf_svc  mobile_usb;
extern struct msf_svc  mobile_dial;
extern struct msf_svc  mobile_sms;
extern struct msf_svc* mobile_module[];

struct msf_svc* mobile_module[] = {
    &mobile_usb,
    &mobile_dial,
    &mobile_sms,
    NULL,
};

struct usb_info usb;
struct dial_info dial;
 
rc = mobile_module[MOBILE_MOD_USB]->init(NULL, 0);
mobile_module[MOBILE_MOD_USB]->get_param(&usb, sizeof(usb));
if (rc < 0) {
    return -1;
}
rc = mobile_module[MOBILE_MOD_USB]->start(NULL, 0);
mobile_module[MOBILE_MOD_USB]->get_param(&usb, sizeof(usb));

mobile_module[MOBILE_MOD_DIAL]->init(&usb, sizeof(usb));
mobile_module[MOBILE_MOD_DIAL]->start(NULL, 0);
mobile_module[MOBILE_MOD_DIAL]->get_param(&dial, sizeof(dial));
```

```
事件Event API使用：
https://github.com/wypx/librpc/blob/master/client/src/thread.c

static void tx_thread_callback(void *arg) {
    //do send something
}

void * tx_thread_worker(void *lparam) {
    struct client *rpc = (struct client*)lparam;
    while (!rpc->stop_flag) {
        msf_event_base_dispatch(rpc->tx_evt_base);
    }
    return NULL;
}

s32 thread_init(void) {
    s32 rc = -1;
    rpc->tx_evt_base = msf_event_base_new();
    if (!rpc->tx_evt_base) {
        return -1;
    }
    
    rpc->tx_evt = msf_event_new(rpc->evt_conn.fd, tx_thread_callback,
                    NULL, NULL, &rpc->evt_conn);
    if (!rpc->tx_evt) {
        return -1;
    }

    msf_event_add(rpc->tx_evt_base, rpc->tx_evt);

    rc = pthread_spawn(&rpc->tx_tid, (void*)tx_thread_worker, rpc);
    if (rc < 0) {
        MSF_RPC_LOG(DBG_ERROR, "TX thread create failed, ret(%d), errno(%d).", rc, errno);
        return -1;
    }
}
```
### 硬件平台适配
``` groovy
根据开发者目标平台以的不同，需要进行相应的适配
```

### ___参考文章___
- [LIBMSF架构设计](https://www.cnblogs.com/duanxz/p/3514895.html)
- [微服务架构的六种模式](https://www.cnblogs.com/duanxz/p/3514895.html)
- [微服务架构设计](https://www.cnblogs.com/SUNSHINEC/p/8628661.html)

### *License*

Libmsf is released under the [Gnu 2.0 license](license.txt).
```
/**************************************************************************
*
* Copyright (c) 2017-2019, luotang.me <wypx520@gmail.com>, China.
* All rights reserved.
*
* Distributed under the terms of the GNU General Public License v2.
*
* This software is provided 'as is' with no explicit or implied warranties
* in respect of its properties, including, but not limited to, correctness
* and/or fitness for purpose.
*
**************************************************************************/
```

