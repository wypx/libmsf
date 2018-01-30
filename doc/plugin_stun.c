
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#ifndef __ANDROID__
#include <ifaddrs.h>
#endif
#include <fcntl.h>
#include <netdb.h>
#include <resolv.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <netinet/in.h>
#include <net/if.h>

#include "utils.h"
#include "plugin.h"
#include "plugin_stun.h"


struct stun_param  stun_client;
struct stun_param* stun = &stun_client;

static int  stun_init(void* data, unsigned int len);
static int  stun_deinit(void* data, unsigned int len);
static int  stun_get_param(void* data, unsigned int len);
static int  stun_set_param(void* data, unsigned int len);
static int  stun_start(void* data, unsigned int len);
static int  stun_stop(void* data, unsigned int len);

static int  stun_responce(void* data, unsigned int len);
static int  stun_request(void* data, unsigned int len);
static int  stun_handler(void* data, unsigned int len);


struct plugin plugin_stun = {
	.name		= "plugin_stun",
	.init 		= stun_init,
	.start 		= stun_start,
	.stop 		= stun_stop,
	.get_param 	= stun_get_param,
	.set_param 	= stun_set_param,
	.request 	= stun_request,
	.responce	= stun_responce,
	.handler	= stun_handler,
	.deinit		= stun_deinit,
};

static struct plugin* p = &plugin_stun;

static int  stun_init(void* data, unsigned int len){
	printf("This is func %s, line number %d\n", __FUNCTION__, __LINE__);

	int i = 0;
	memcpy(stun->refer_plugins_name[i++], "plugin_p2p",  strlen("plugin_p2p"));
	memcpy(stun->refer_plugins_name[i++], "plugin_ddns", strlen("plugin_ddns"));

	stun->refer_plugins_num = i;
	
	return 0;
}

static int  stun_deinit(void* data, unsigned int len){
	printf("This is func %s, line number %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int stun_get_param(void* data, unsigned int len){
	printf("This is func %s, line number %d\n", __FUNCTION__, __LINE__);

	return 0;
}

static int stun_set_param(void* data, unsigned int len){
	printf("This is func %s, line number %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int stun_start(void* data, unsigned int len){
	printf("This is func %s, line number %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int stun_stop(void* data, unsigned int len){
	printf("This is func %s, line number %d\n", __FUNCTION__, __LINE__);

	struct plugin_message m;
	memset(&m, 0, sizeof(m));

	m.command = 100;
	m.resp = 1;
	memcpy(m.dest, "plugin_p2p", strlen("plugin_p2p"));
	memcpy(m.rest, "plugin_stun", strlen("plugin_stun"));
	
	p->phost->observer(&m, sizeof(m));
	
	return 0;
}


static int stun_responce(void* data, unsigned int len) {
	printf("%s recv stun_responce \n", __FUNCTION__);

	struct plugin_message* m = (struct plugin_message*)data;

	printf("stun_responce command = %d\n", m->command);
	printf("stun_responce rest = %s\n", m->rest);
	printf("stun_responce dest = %s\n", m->dest);
	return 0;
}

static int stun_request(void* data, unsigned int len) {

	
	return 0;
}
static int stun_handler(void* data, unsigned int len) {
	printf("This is func %s, line number %d\n", __FUNCTION__, __LINE__);
	return 0;
}



