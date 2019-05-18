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
#define MSF_DLERROR() dlerror()
#endif

typedef s32 (*svc_func)(void *data, u32 len);

struct msf_svc {
    svc_func init;
    svc_func deinit;
    svc_func start;
    svc_func stop;
    svc_func get_param;
    svc_func set_param;
    svc_func msg_handler;
} MSF_PACKED_MEMORY;

typedef struct msf_svc msf_plugin;
typedef struct msf_svc msf_module;

struct svcinst {
    u32 svc_state;
    s8  svc_name[64];
    s8  svc_lib[64];

    u32 svc_deplib_num;
    s8  *svc_deplib[8];

    MSF_DLHANDLE svc_handle;
    struct msf_svc *svc_cb;
} MSF_PACKED_MEMORY;

s32 msf_svc_stub(const s8 *func, ...);
void *msf_load_dynamic(const s8 *path);
void *msf_load_symbol(void *handler, const s8 *symbol);
s32 msf_svcinst_init(struct svcinst *svc);

/** if func is null, stub func is called
  * and print debug info
  */
#define MSF_VA_ARGS(...) , ##__VA_ARGS__
#define MFS_FUNCHK(func, ...) ((!func)?(msf_svc_stub(__FUNCTION__ MSF_VA_ARGS(__VA_ARGS__))):(func(__VA_ARGS__)))


