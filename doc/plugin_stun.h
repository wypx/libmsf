
#ifndef _PLUGIN_STUN_H
#define _PLUGIN_STUN_H

#ifdef __cplusplus
extern "C" {
#endif

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

#include "network.h"


#define MAX_REFER_PLUGINS 12

struct stun_param {
	int	 fd;
	char ip[32];
	unsigned short port;
	unsigned short reserved;

	int	 state;
	int  refer_plugins_num;
	char refer_plugins_name[16][MAX_REFER_PLUGINS];
} ;


#ifdef __cplusplus
}
#endif
#endif

