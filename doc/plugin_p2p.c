#include "utils.h"
#include "plugin.h"

static int  p2p_init(void* data, unsigned int len);
static int  p2p_deinit(void* data, unsigned int len);
static int  p2p_get_param(void* data, unsigned int len);
static int  p2p_set_param(void* data, unsigned int len);
static int  p2p_start(void* data, unsigned int len);
static int  p2p_stop(void* data, unsigned int len);

static int  p2p_responce(void* data, unsigned int len);
static int  p2p_request(void* data, unsigned int len);


struct plugin plugin_p2p = {
	.name		= "plugin_p2p",
	.init 		= p2p_init,
	.start 		= p2p_start,
	.stop 		= p2p_stop,
	.get_param 	= p2p_get_param,
	.set_param 	= p2p_set_param,
	.request 	= p2p_request,
	.responce	= p2p_responce,
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

static int p2p_responce(void* data, unsigned int len) {
	printf("This is func %s, line number %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int p2p_request(void* data, unsigned int len) {

	
	printf("%s recv request \n", __FUNCTION__);

	struct plugin_message* m = (struct plugin_message*)data;

	printf("p2p_request command = %d\n", m->command);
	printf("p2p_request rest = %s\n", m->rest);
	printf("p2p_request dest = %s\n", m->dest);

	char rest[16];
	char dest[16];

	memset(rest, 0, sizeof(rest));
	memset(dest, 0, sizeof(dest));

	memcpy(dest, m->rest, strlen(m->rest));
	memcpy(rest, m->dest, strlen(m->dest));
	memcpy(m->rest, rest, sizeof(m->rest));
	memcpy(m->dest, dest, sizeof(m->dest));

	m->command = 101;
	m->resp = 0;
	p->phost->observer(m, sizeof(struct plugin_message));
	
	return 0;
}

