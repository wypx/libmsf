# **Linux平台微服务框架-libmsf**
MSF (Mircro Service Framework) acting as mircro service process framework, provide infrastructure and provide service monitoring etc.

基于Linux C的为微服务管理框架
# __主要特性包括:__
1. 为各个模块提供基础设施模块 比如`内存` `日志` `网络` `事件` `信号`等基础库
2. 提供微服务`进程动态拉起` `动态监控`框架，实现微服务进程快速秒级拉起
3. 提供微服务`so动态加载` `动态卸载`做到插件化的即插即用
4. 提供各个微服务进程的`内存监控`等亚健康状态监控，达到实时告警通知
5. 提供微服务配置动态解析，热切换微服务进程，实现平滑升级过度

### __快速开始__
### 一. 开发环境准备
1. 安装Ubuntu，Debian等Linux,ARM环境

测试环境（Linux KaliCI 4.19.0-kali3-amd64）：gcc version 8.2.0 (Debian 8.2.0-14) <br/>
测试环境（Linux raspberrypi 4.14.52-v7+）：gcc version 6.4.1 20171012 (Linaro GCC 6.4-2017.11) <br/>

### 二. 编译样例程序
1. ***下载该开源程序***

将下载到的压缩包在Linux上解压缩

2. ***进入编译目录目录***

运行如下命令: <br/>
下载libmsf微服务框架：<br/>
git clone https://github.com/wypx/libmsf/ <br/>
root@KaliCI:/media/psf/tomato/mod/libmsf make <br/>
Make Subdirs msf <br/>
make[1]: Entering directory '/media/psf/tomato/mod/libmsf/msf <br/>
arm-linux-gnueabihf-gcc lib/src/msf_log.o<br />
................. <br/>
make[1]: Leaving directory '/media/psf/tomato/mod/libmsf/msf_daemon <br/>
下载微服务通信框架：<br/>
git clone https://github.com/wypx/librpc/ <br/>
root@KaliCI:/media/psf/tomato/mod/librpc/server make <br/>
arm-linux-gnueabihf-gcc bin/src/conn.o <br/>
arm-linux-gnueabihf-gcc bin/src/config.o <br/>
....... <br/>

3. ___检查生成的样例程序___
编译产生:
msf_agent  各个服务进程之间的通信代理服务端程序
msf_dlna   测试程序 - 独立微服务进程客户端DLNA
msf_upnp   测试程序 - 独立微服务进程客户端UPNP
msf_daemon 用于守护监控代理进程msf_agent
msf_shell  壳子程序，用于记载配置文件中的插件

libipc.so 提供给各个微服务进程连接的通信代理客户端库
libipc.so 提供给各个微服务进程的基础设施库
          包括：网络 管道 Epoll等事件驱动 日志 共享内存 内存池 
          串口通信 线程 进程 CPU工具 文件 加密 微服务框架 定时器

root@KaliCI:/media/psf/tomato/mod/librpc/client# ls ../../../packet/binary/<br />
msf_agent  msf_daemon  msf_dlna  msf_shell  msf_upnp<br />

root@KaliCI:/media/psf/tomato/mod/librpc/client# ls ../../../packet/library/<br />
libipc.so  libmsf.so<br />

### 四. ___运行样例程序___
1. 执行样例程序
$ ./msf_agent &
$ ./msf_upnp &
$ ./msf_dlna &
2. 查看运行日志
3. API和使用方式
demo.c程序中提供了该API的使用方式

```
参考demo程序：
https://github.com/wypx/libmsf/blob/master/msf/src/msf_svc.c
https://github.com/wypx/libmsf/blob/master/msf_shell/src/msf_shell.c

s32 service_init(void) {

    u32 svc_idx;
    struct msf_svc *svc_cb;

    for (svc_idx = 0; svc_idx < g_proc->proc_svc_num; svc_idx++) {
        MSF_SHELL_LOG(DBG_ERROR, "Start to load svc %d.", svc_idx);
        if (msf_svcinst_init(&(g_proc->proc_svcs[svc_idx])) < 0) {
            MSF_SHELL_LOG(DBG_ERROR, "Failed to load svc %d.", svc_idx);
            //return -1;
        };
    }

    for (svc_idx = 0; svc_idx < g_proc->proc_svc_num; svc_idx++) {
        svc_cb = g_proc->proc_svcs[svc_idx].svc_cb;
        if (svc_cb->init(NULL, 0) < 0) {
            MSF_SHELL_LOG(DBG_ERROR, "Failed to init svc %d.", svc_idx);
            return -1;
        }
    }
    return 0;
}
```

4. ___硬件平台适配___
根据开发者目标平台以的不同，需要进行相应的适配。

代码解析：<br/>
博客文章：<a href="http://luotang.me" target="_blank">http://luotang.me</a>

参考文章：<br/>
https://www.cnblogs.com/duanxz/p/3514895.html <br/>
https://www.cnblogs.com/SUNSHINEC/p/8628661.html <br/>

