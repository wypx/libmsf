#include "utils.h"
#include "plugin.h"

typedef enum
{
	UNAT_OK,						
	UNAT_ERROR,						
	UNAT_NOT_OWNED_PORTMAPPING,		
	UNAT_EXTERNAL_PORT_IN_USE,		
	UNAT_NOT_FOUND					
}UNAT_STAT;

typedef enum
{
	UNAT_TCP,						
	UNAT_UDP,						
} UNAT_PROTOCOL;

typedef enum
{
	UNAT_ADD,
	UNAT_DELETE
} UNAT_ACTYPE;


#define NAME_LEN 32
typedef struct {
	unsigned int	enabled;
	char 		  	desc[NAME_LEN];
	unsigned int  	innerPort;
	UNAT_PROTOCOL  	protocol;
	unsigned int	b_exit_remove;
}UNAT_PARAM;


typedef struct {

	unsigned int  enable_discovery;
	char		  friendName[32];
	char		  manufacturer_name[32];
	char		  manufacturer_url[32];
	char		  model_name[32];
	char		  model_description[32];
	char		  model_url[32];
	char		  uuid[128];

	char		  serial[32];
	int			  model_number;
	
	unsigned int  http_port;
	unsigned int  https_port;
	unsigned int  notify_interval;
	unsigned int  system_uptime;/* Report system uptime instead of daemon uptime */
	char		  ssdpdsocket[32];/* minissdpdsocket:default /var/run/minissdpd.sock*/

	unsigned int  enable_nat;
	char		  inner_ip[32];
	char		  extern_ip[32];
	
	UNAT_PARAM 	  nat[4];
	
}UPNP_PARAM;


static int  nat_init(void* data, unsigned int len);
static int  nat_deinit(void* data, unsigned int len);
static int  nat_get_param(void* data, unsigned int len);
static int  nat_set_param(void* data, unsigned int len);
static int  nat_start(void* data, unsigned int len);
static int  nat_stop(void* data, unsigned int len);



struct plugin plugin_nat = {
	.name		= "plugin_nat",
	.init 		= nat_init,
	.start 		= nat_start,
	.stop 		= nat_stop,
	.get_param 	= nat_get_param,
	.set_param 	= nat_set_param,
	.observer 	= NULL,
	.request 	= NULL,
	.responce	= NULL,
	.deinit		= nat_deinit,
};


static int  
nat_init(void* data, unsigned int len){
	printf("This is func %s, line number %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int  
nat_deinit(void* data, unsigned int len){
	printf("This is func %s, line number %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int  
nat_get_param(void* data, unsigned int len){
	printf("This is func %s, line number %d\n", __FUNCTION__, __LINE__);

	return 0;
}

static int  
nat_set_param(void* data, unsigned int len){
	printf("This is func %s, line number %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int  
nat_start(void* data, unsigned int len){
	printf("This is func %s, line number %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int  
nat_stop(void* data, unsigned int len){
	printf("This is func %s, line number %d\n", __FUNCTION__, __LINE__);
	return 0;
}

