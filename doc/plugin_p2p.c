#include "utils.h"
#include "plugin.h"

static int  p2p_init(void* data, unsigned int len);
static int  p2p_deinit(void* data, unsigned int len);
static int  p2p_get_param(void* data, unsigned int len);
static int  p2p_set_param(void* data, unsigned int len);
static int  p2p_start(void* data, unsigned int len);
static int  p2p_stop(void* data, unsigned int len);


struct plugin plugin_p2p = {
	.name		= "plugin_p2p",
	.init 		= p2p_init,
	.start 		= p2p_start,
	.stop 		= p2p_stop,
	.get_param 	= p2p_get_param,
	.set_param 	= p2p_set_param,
	.deinit		= p2p_deinit,
};

static struct plugin* p = &plugin_p2p;

static int  p2p_init(void* data, unsigned int len) {
	printf("This is func %s, line number %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int p2p_deinit(void* data, unsigned int len){
	printf("This is func %s, line number %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int p2p_get_param(void* data, unsigned int len) {
	printf("This is func %s, line number %d\n", __FUNCTION__, __LINE__);

	return 0;
}

static int p2p_set_param(void* data, unsigned int len) {
	printf("This is func %s, line number %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int p2p_start(void* data, unsigned int len) {
	printf("This is func %s, line number %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int p2p_stop(void* data, unsigned int len) {
	printf("This is func %s, line number %d\n", __FUNCTION__, __LINE__);
	return 0;
}


