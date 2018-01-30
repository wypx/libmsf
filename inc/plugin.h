/*
	微内核: 核心层的设计和实现
	1. 插件的加载, 检测,初始化. 插件的加载利用Linux共享库的动态加载技术
	2. 服务的注册, 服务的注册与调用采用表驱动的方法. 核心层中维护一个服务注册表.
	3. 服务的调用
	4. 服务的管理

	框架分为宿主程序和插件对象两部分
	两部分交互基于一种公共的通信契约
	宿主程序可以独立存在


	使用插件的原因
	可以在无需对程序进行重新编译和发布的条件下扩展程序的功能
	可以在不需要程序源代码的环境下为程序增加新的功能
	在一个程序的业务逻辑不断发生改变、新的规则频频加入时能够灵活适应
	
	插件要求的基础设施统一, 插件之间通讯, 动态壮哉, 依赖解耦
	利用观察者模式,在宿主中加载插件后,便能实现事件注册,进而实现插件之间的通信。

	宿主程序(host)和插件(plugin)
	
	Discovery
	Registration
	Application hooks to which plugins attach
	Exposing application capabilities back to plugins

	Compile: gcc -fPIC -shared xxx.c -o xxx.so 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "list.h"
/*#include <kernel_list.h>*/


typedef enum {
	PLUG_UNINSTALLED,
	PLUG_INSTALLED,
	PLUG_RESOLVED,
	PLUG_STARTING,
	PLUG_STOPPING,
	PLUG_ACTIVE,
} plug_stat;

typedef int (*plug_func)(void* data, unsigned int len);

struct plugin {
	
	struct list_head head;

	int 		plugid;		/* unique plugid */
	int 		major;
	int 		minor;
	char 		ver[16];
	char		name[16];  	/* nat.so nat.dll */
	char		author[16];
	char		desc[16];
	
	DLHANDLE	handle;
	int			loadtype;

	/* Hooks and capabilities */
    int 		hooks;
	plug_stat	stat;

	plug_func	init;
	plug_func	start;
	plug_func	stop;
	plug_func	get_param;
	plug_func	set_param;
	plug_func	handler;
	plug_func	request;	//send request message to other plungins
	plug_func	responce; 	//send responce message to other plungins
	plug_func	deinit;
	
	struct plugin_host*	phost;
}__attribute__((__packed__));


struct plugin_host {
	int			number;
	int			state;
	plug_func	observer; 	//observer called synchronously after a plugin state change.
	struct list_head plugins;
}__attribute__((__packed__));

struct plugin_message {
	char	rest[16];
	char	dest[16];
	int		command;
	int		resp;
	char 	payload[128];
}__attribute__((__packed__));



/* Plugin types */
#define PLUGIN_STATIC     0   /* built-in into core */
#define PLUGIN_DYNAMIC    1   /* shared library     */

//Plugin FramWork 

#define CP_SP_UPGRADE 0x01
#define CP_SP_STOP_ALL_ON_UPGRADE 0x02
#define CP_SP_STOP_ALL_ON_INSTALL 0x04
#define CP_SP_RESTART_ACTIVE 0x08


typedef enum  {
	CP_OK = 0,
	CP_ERR_RESOURCE,	/** Not enough memory or other operating system resources available */
	CP_ERR_UNKNOWN,		/** The specified object is unknown to the framework */
	CP_ERR_IO,			/** An I/O error occurred */
	CP_ERR_MALFORMED,	/** Malformed plug-in descriptor was encountered when loading a plug-in */
	CP_ERR_CONFLICT,	/** Plug-in or symbol conflicts with another plug-in or symbol. */
	CP_ERR_DEPENDENCY, 	/** Plug-in dependencies could not be satisfied. */
	CP_ERR_RUNTIME, 	/** Plug-in runtime signaled an error. */
	
}PFState;

int plugins_size(void);
struct plugin* plugin_lookup(const char* name);

int plugin_install(const char* name, int type);
int plugin_uninstall(const char* name);
int plugin_start(const char* name);
int plugin_stop(const char* name);

int plugins_reg_observer(plug_func listener);
int plugins_unreg_observer(plug_func listener);

int plugins_scaning(char* dir, int flags);

int plugins_init(void);
int plugins_start(void);
int plugins_stop(void);
int plugins_uninstall(void);


