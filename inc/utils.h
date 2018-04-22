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
	RTLD_LAZY 	暂缓决定,等有需要时再解出符号 
　　RTLD_NOW 	立即决定,返回前解除所有未决定的符号 
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

#include <stdio.h>
#include <dlfcn.h>

/* Linux dl API*/
#ifndef WIN32  
#define DLHANDLE void*
#define DLOPEN_L(name) dlopen((name), RTLD_LAZY | RTLD_GLOBAL)
#define DLOPEN_N(name) dlopen((name), RTLD_NOW)


#if defined(WIN32)
#define DLSYM(handle, symbol) 	GetProcAddress((handle), (symbol))
#define DLCLOSE(handle)			FreeLibrary(handle)
#else
#define DLSYM(handle, symbol) 	dlsym((handle), (symbol))
#define DLCLOSE(handle) 		dlclose(handle)
#endif
#define DLERROR() dlerror()
#endif

void* plugin_load_dynamic(const char* path);

void* plugin_load_symbol(void* handler, const char* symbol);



