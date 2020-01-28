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
#ifndef __MSF_PLUGIN_H__
#define __MSF_PLUGIN_H__

#include <unistd.h>
#include <stdint.h>
#include <dlfcn.h>
#include <iostream>
#include <memory>
#include <list>
#include <vector>
#include <map>

#include <base/Noncopyable.h>
using namespace MSF::BASE;

namespace MSF {
namespace PLUGIN {

#ifndef PLUGIN_API
  #ifdef WIN32
    #define PLUGIN_API __declspec(dllimport)
  #else
    #define PLUGIN_API
  #endif
#endif

/* Linux dl API*/
#if defined(WIN32)
#define MSF_DLHANDLE                HMODULE
#define MSF_DLOPEN_L(name)          LoadLibraryA(name);
#define MSF_DLOPEN_N(name)          LoadLibraryA(name);
#define MSF_DLSYM(handle, symbol)   GetProcAddress((handle), (symbol))
#define MSF_DLCLOSE(handle)         FreeLibrary((HMODULE)handle)
#define MSF_DLERROR()               GetLastError()
#define MSF_LIB_EXTENSION           "dll"
static std::string dynamicLibraryExtension("dll");
#else
#define MSF_DLHANDLE                void*
#define MSF_DLOPEN_L(name)          dlopen((name), RTLD_LAZY | RTLD_GLOBAL)
#define MSF_DLOPEN_N(name)          dlopen((name), RTLD_NOW)
#define MSF_DLSYM(handle, symbol)   dlsym((handle), (symbol))
#define MSF_DLCLOSE(handle)         dlclose(handle)
#define MSF_DLERROR()               dlerror()
#define MSF_LIB_EXTENSION           "so"     /* linux and freebsd */
static std::string dynamicLibraryExtension("so");
#endif

/* macos #define MSF_LIB_EXTENSION "dylib" */


#define MSF_SVC_NOT_FOUND       (401)
#define MSF_MAX_PATH_LEN        (64)

#define MSF_RMMOD_MODE_NONE     0x0
#define MSF_RMMOD_MODE_WAIT     0x1
#define MSF_RMMOD_MODE_FORCE    0x2


 /** if func is not defined, stub func is called
  *  and print some debug info
  */
int PluginFuncStub(const char *func, ...);
#define MSF_VA_ARGS(...)      , ##__VA_ARGS__
#define MFS_FUNCHK(func, ...) ((!func)?(PluginFuncStub(__FUNCTION__ MSF_VA_ARGS(__VA_ARGS__))): \
                              (func(__VA_ARGS__)))

typedef int (*PluginFunc)(void *data, const uint32_t len);

class Plugin : public Noncopyable /* : public std::enable_shared_from_this<Plugin>  */
{
   public:
      uint32_t getPlugState() const { return _state; }
      std::string  getPluginName() const { return _name; }
      std::vector<std::string> getPluginDepLibs() const { return _deplibs; }

      Plugin() = default;
      virtual ~Plugin();
      MSF_DLHANDLE _handle;

   private:
      uint32_t _state;
      std::string _name;
      std::pair<std::string, std::string> _version;
      std::vector<std::string> _deplibs;

      PluginFunc _init;
      PluginFunc _deinit;
      PluginFunc _start;
      PluginFunc _stop;
      PluginFunc _getParam;
      PluginFunc _setParam;
      PluginFunc _msgHandler;
};

class PluginManager : public Noncopyable
{
   public:
      PluginManager & getInstance()
      {
         static PluginManager instance;
         return instance;
      }
      virtual ~PluginManager();
      
      /** 
       * \brief Load a plugin given it's pluginPath
       * \param pluginPath Path for the plugin, including plugin name. File extension
       * may be included, but is discouraged for better cross platform code.
       * If file extension isn't present on the path, Pluma will deduce it
       * from the operating system.
       * \return True if the plugin is successfully loaded.
       * */
      bool load(const std::string& pluginPath);
       /** 
       * \brief Load a plugin from a given pluginPath
       * \param folder The pluginPath folder.
       * \param pluginName Name of the plugin. File extension may be included,
       * but is discouraged for better cross platform code.
       * If file extension is omitted, we will deduce it from the operating system.
       * \return True if the plugin is successfully loaded.
       * */
      bool load(const std::string& folder, const std::string& pluginName);
      /** 
       * \brief Load all plugins from a given folder
       * \param folder Path for the folder where the plug-ins are.
       * \param recursive If true it will search on sub-folders as well
       * \return Number of successfully loaded plug-ins.
       * */
      int loadFromFolder(const std::string& folder, bool recursive = false);
      /** 
       * \param pluginName Name or path of the plugin.
       * */
      bool unload(const std::string& pluginName);
      void unloadAll();

      std::string resolvePathExtension(const std::string& path);
      void getLoadedPlugins(std::vector<const std::string*>& pluginNames) const;
      bool isLoaded(const std::string& pluginName) const;

      /* Do insmod and rmmod kernel ko */
      int driverInsmod(const std::string & ko_path);
      int driverRmmod(const std::string & ko_path, const int mode);
   protected:
      explicit PluginManager() = default;
   private:
      /* Get the plugin name (without extension) from its path */
      static std::string getPluginName(const std::string& path);

      std::map<std::string, Plugin*> _plugins;
};

} /**************************** end namespace BASE ****************************/
} /**************************** end namespace MSF  ****************************/
#endif
