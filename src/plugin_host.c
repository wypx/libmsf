/**************************************************************************
*
* Copyright (c) 2017, luotang.me <wypx520@gmail.com>, China.
* All rights reserved.
*
* Distributed under the terms of the GNU General Public License v2.
*
* This software is provided 'as is' with no explicit or implied warranties
* in respect of its properties, including, but not limited to, correctness
* and/or fitness for purpose.
*
**************************************************************************/
#include "utils.h"
#include "plugin.h"


#include <unistd.h>   
#include <dirent.h>  
#include <sys/stat.h>  


struct plugin_host  host;
struct plugin_host* phost = &host;

extern int plugin_uninstall_p(struct plugin* p);
extern struct plugin* plugin_loading(const char* name, int type);

int plugins_size(void) {
	return list_size(&phost->plugins);
}

int plugins_reg_observer(plug_func listener) {
	phost->observer = listener;
	return 0;
}
int plugins_unreg_observer(plug_func listener) {
	return 0;
}

int plugins_scaning(char* dir, int flags) {
	
	#define FULLPATH_LEN 256
	
	char cwd[FULLPATH_LEN];
	
#ifdef WIN32	
		DWORD res = ::GetCurrentDirectoryA(256, cwd);	  
#else    
		cwd[0] = '\0';	 
		char * res = getcwd(cwd, 256);	 
#endif
	
	struct stat s;
	if (stat(dir, &s) != 0)
		return -1;
	
	DIR* dir_info = NULL;
	struct dirent* dir_entry = NULL;
	char path[FULLPATH_LEN + 6];
	int last_slash = 0;

	dir_info = opendir(dir);
	if (!dir_info)
		return -1;
	
	if (dir[strlen(dir)-1] == '/')
			last_slash = 1;
	while ((dir_entry = readdir(dir_info)) != NULL) {
		struct stat s;
		if (dir_entry->d_name[0] == '.')
			continue;

		printf("%s\n", dir_entry->d_name);
	#if 0	
		if( last_slash )
			snprintf(path, FULLPATH_LEN, "%s%s", dir, dir_entry->d_name);
		else
			snprintf(path, FULLPATH_LEN, "%s/%s", dir, dir_entry->d_name);

		//int res = get_plugin_name(dir_entry->d_name);

		//pf_install_plugin(path, dir_entry->d_name );        
	#endif
	}
	closedir(dir_info);
	return 0;
}

int plugins_init(void) {

	phost->number = 0;
	phost->state = CP_ERR_UNKNOWN;
	INIT_LIST_HEAD(&phost->plugins);

	return 0;
}

int plugins_start(void) {
	struct plugin*		p = NULL;
	struct list_head*	head = NULL;

	list_for_each(head, &phost->plugins) {
	   p = list_entry(head, struct plugin, head);
	
	   if (p->start)
	    p->start(NULL, 0);
	}
	return 0;
}

int plugins_stop(void) {
	struct plugin* 		p = NULL;
    struct list_head*   head = NULL;

	list_for_each(head, &phost->plugins) {
       p = list_entry(head, struct plugin, head);
       if (p->stop) {
            p->stop(NULL, 0);
       }
    }
	return 0;
}

int plugins_uninstall(void) {

	struct plugin* p 	 = NULL;
	struct list_head* 	tmp  = NULL;
    struct list_head*   head = NULL;

	/* plugin interface it self */
    list_for_each_safe(head, tmp, &phost->plugins) {
        p = list_entry(head, struct plugin, head);
        plugin_uninstall_p(p);
    }
	return 0;
}


