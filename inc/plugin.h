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
	   插件之间采用组件通信/路由机制, 参考[luotang.me---libipc]
	6. 服务事件的广播和订阅(这个目前还没有考虑要支持)
	   利用观察者模式,在宿主中加载插件后,实现事件注册,进而插件通信其他


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
	//plug_func	request;	//send request message to other plungins
	//plug_func	responce; 	//send responce message to other plungins
	plug_func	deinit;
	
	//struct plugin_host*	phost;
}__attribute__((__packed__));


struct plugin_host {
	int			number;
	int			state;
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


