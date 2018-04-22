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


#include "utils.h"
#include "file.h"
#include "plugin.h"

extern struct plugin_host* phost;

struct plugin* plugin_loading(const char* name, int type)
{
	int   ret = -1;
    void* handler = NULL;
	char  path[1024];
	
	struct file_info f_info;
	
    struct list_head* head 	= NULL;
    struct plugin* tmp 	= NULL;
    struct plugin* p 	= NULL;

	memset(&f_info, 0, sizeof(f_info));
	memset(path, 0, sizeof(path));

   	if (PLUGIN_DYNAMIC == type) {
	   snprintf(path, sizeof(path)-1, "plugins/%s.so", name);
	   ret = file_get_info(path, &f_info, FILE_READ);
	   if (ret == -1 || f_info.is_directory == 0) {
	   		//memset(path, 0, sizeof(path));
	      // snprintf(path, 1024, "%s", name);
	   }
	   if (!(handler = plugin_load_dynamic(path))) {
	   		return NULL;
	   }

	   if (!(p  = plugin_load_symbol(handler, name))) {
		  DLCLOSE(handler);
		  return NULL;

	   };

	   /* Make sure this is not loaded twice (ref #218) */
	   list_for_each(head, &phost->plugins) {
		   tmp = list_entry(head, struct plugin, head);
		   if (tmp->loadtype == PLUGIN_STATIC &&
			   strcmp(tmp->name, p->name) == 0) {
			   printf("Plugin '%s' have been built-in.\n", tmp->name);
			   DLCLOSE(handler);
			   return NULL;
		   }
	   }  
	   p->loadtype = PLUGIN_DYNAMIC;
	   p->handle	 = handler;
   }
   else if (type == PLUGIN_STATIC) {
	   p = (struct plugin *) name;
	   p->loadtype = PLUGIN_STATIC;
   }
   
  // p->phost = phost;

   printf("plugin_load : '%s' successful\n", path);

   return p;

}


int plugin_install(const char* name, int type) {

	if (!name || !phost) 
		return -1;
	
	struct plugin* p = plugin_loading(name, type);
	if (p) {
		if (type == PLUGIN_DYNAMIC) {
		   /* Add Plugin to the end of the list */
		   list_add(&p->head, &phost->plugins);
	    }
	} else {
		return -1;
	}
	if (p->init)
	  p->init(NULL, 0);
	
	return 0;
}

int plugin_start(const char* name){
	
	struct plugin* p = plugin_lookup(name);

	return (!p) ? -1 : p->start(NULL, 0);
}

int plugin_stop(const char* name) {
	
	struct plugin* p = plugin_lookup(name);
	
	return (!p) ? -1 : p->stop(NULL, 0);
}

int plugin_uninstall(const char* name) {
	
	struct plugin* p = plugin_lookup(name);

	if (p) {
		if (p->deinit)
		 p->deinit(NULL, 0);
		
		list_del(&p->head);
		if (p->loadtype == PLUGIN_DYNAMIC) {
			DLCLOSE(p->handle);
		}
	} else {
		return -1;
	}
	return 0;
}

int plugin_uninstall_p(struct plugin* p) {
	if (p) {
		if (p->deinit)
		  p->deinit(NULL, 0);
		
		list_del(&p->head);
		if (p->loadtype == PLUGIN_DYNAMIC) {
			DLCLOSE(p->handle);
		}
	} else {
		return -1;
	}
	return 0;
}


struct plugin* plugin_lookup(const char* name) {
	
	if (!name || !phost) 
		return NULL;
	
	struct list_head* head = NULL;
	struct plugin* p = NULL;
	
	list_for_each(head, &phost->plugins) {
		p = list_entry(head, struct plugin, head);
		if(0 == strcmp(p->name, name))
			return p;
	}

	return NULL;
}


