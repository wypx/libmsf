/**************************************************************************
*
* Copyright (c) 2017-2018, luotang.me <wypx520@gmail.com>, China.
* All rights reserved.
*
* Distributed under the terms of the GNU General Public License v2.
*
* This software is provided 'as is' with no explicit or implied warranties
* in respect of its properties, including, but not limited to, correctness
* and/or fitness for purpose.
*
**************************************************************************/
/*  Open Shared Object
    RTLD_LAZY   暂缓决定,等有需要时再解出符号 
　　RTLD_NOW  立即决定,返回前解除所有未决定的符号 
　　RTLD_LOCAL  与RTLD_GLOBAL作用相反,
                动态库中定义的符号不能被其后打开的其它库重定位
                如果没有指明是RTLD_GLOBAL还是RTLD_LOCAL,则缺省为RTLD_LOCAL.
　　RTLD_GLOBAL 允许导出符号  动态库中定义的符号可被其后打开的其它库重定位
　　RTLD_GROUP 
　　RTLD_WORLD 
    RTLD_NODELETE:在dlclose()期间不卸载库,
                并且在以后使用dlopen()重新加载库时不初始化库中的静态变量
                这个flag不是POSIX-2001标准。 
    RTLD_NOLOAD:不加载库
                可用于测试库是否已加载(dlopen()返回NULL说明未加载,
                否则说明已加载）,也可用于改变已加载库的flag
                如:先前加载库的flag为RTLD＿LOCAL
                用dlopen(RTLD_NOLOAD|RTLD_GLOBAL)后flag将变成RTLD_GLOBAL
                这个flag不是POSIX-2001标准
    RTLD_DEEPBIND:在搜索全局符号前先搜索库内的符号,避免同名符号的冲突
                这个flag不是POSIX-2001标准。     
*/

/** 微内核的插件架构 -- OSGI的 "微内核+系统插件+应用插件" 结构
    框架分为宿主程序和插件对象两部分, 宿主可以独立存在，可以动态加载插件.
    1. 插件的加载
       利用Linux共享库的动态加载技术
    2. 服务的注册
       采用表驱动的方法. 核心层中维护一个服务注册表, 隐藏服务接口具体实现.
    3. 服务的调用 
       插件宿主的回调插件提供的服务接口函数
    4. 服务的管理
       插件宿主管理插件的生命周期, 处理插件的错误事件
    5. 服务的通信
       插件之间采用组件通信/路由机制, 参考[luotang.me-librpc]
    6. 服务事件的广播和订阅(这个目前还没有考虑要支持)
       利用观察者模式,在宿主中加载插件后,实现事件注册,进而插件通信其他
    Discovery
    Registration
    Application hooks to which plugins attach
    Exposing application capabilities back to plugins
    Compile: gcc -fPIC -shared xxx.c -o xxx.so 
*/
#ifndef __MSF_SVC_H__
#define __MSF_SVC_H__

#include <msf_utils.h>
#include <dlfcn.h>

/* Linux dl API*/
#ifndef WIN32  
#define MSF_DLHANDLE void*
#define MSF_DLOPEN_L(name) dlopen((name), RTLD_LAZY | RTLD_GLOBAL)
#define MSF_DLOPEN_N(name) dlopen((name), RTLD_NOW)

#if defined(WIN32)
#define MSF_DLSYM(handle, symbol)   GetProcAddress((handle), (symbol))
#define MSF_DLCLOSE(handle)         FreeLibrary(handle)
#else
#define MSF_DLSYM(handle, symbol)   dlsym((handle), (symbol))
#define MSF_DLCLOSE(handle)         dlclose(handle)
#endif
#define MSF_DLERROR()               dlerror()
#endif

#define MSF_SVC_NOT_FOUND       (401)
#define MSF_MAX_PATH_LEN        (64)

#define MSF_RMMOD_MODE_NONE     0x0
#define MSF_RMMOD_MODE_WAIT     0x1
#define MSF_RMMOD_MODE_FORCE    0x2

typedef s32 (*svc_func)(void *data, u32 len);

struct msf_svc {
    svc_func init;
    svc_func deinit;
    svc_func start;
    svc_func stop;
    svc_func get_param;
    svc_func set_param;
    svc_func msg_handler;
};

typedef struct msf_svc msf_plugin;
typedef struct msf_svc msf_module;

struct svcinst {
    u32 state;
    s8  svc_name[MSF_MAX_PATH_LEN];
    s8  svc_lib[MSF_MAX_PATH_LEN];

    u32 svc_deplib_num;
    s8  *svc_deplib[8];

    MSF_DLHANDLE svc_handle;
    struct msf_svc *svc_cb;
};

s32 msf_svc_stub(const s8 *func, ...);
void *msf_load_dynamic(const s8 *path);
void *msf_load_symbol(void *handler, const s8 *symbol);
s32 msf_svcinst_init(struct svcinst *svc);

s32 msf_insmod(const s8 *path);
s32 msf_rmmod(const s8 *path, s32 mode);

/** if func is null, stub func is called
  * and print debug info
  */
#define MSF_VA_ARGS(...) , ##__VA_ARGS__
#define MFS_FUNCHK(func, ...) ((!func)?(msf_svc_stub(__FUNCTION__ MSF_VA_ARGS(__VA_ARGS__))):(func(__VA_ARGS__)))

#endif
