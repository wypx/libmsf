#include "utils.h"
#include "plugin.h"
#include "plugin_stub.h"

int test2(int a, int b) {
	printf("test a = %d b = %d\n", a, b);
	return 0;
}
int ATTRIBUTE_WIKE test(int a, int b);

int main() {

	printf("############## plugin_host_scaning ###############\n");
	plugins_scaning("plugins", 0);

	plugins_init();

	printf("\nplugins_size = %d\n", plugins_size());

	printf("\n############## plugin_install ####################\n");
	plugin_install("plugin_p2p", PLUGIN_DYNAMIC);
	plugin_install("plugin_stun", PLUGIN_DYNAMIC);


	printf("\nplugins_size = %d\n", plugins_size());

	printf("\n############## plugin_start #####################\n");
	plugin_start("plugin_p2p");
	plugin_start("plugin_stun");


	printf("\n############## plugins_start #####################\n");
	plugins_start();


	printf("\n############## plugin_lookup #####################\n");
	struct plugin* p = plugin_lookup("plugin_p2p");
	p->get_param(NULL, 0);
	p->set_param(NULL, 0);
	p->deinit(NULL, 0);

	FUNCK(test,  1, 2);
	FUNCK(test2, 1, 2);


	printf("\n############## plugin_stop ######################\n");
	plugin_stop("plugin_p2p");
	plugin_stop("plugin_stun");

	printf("\n############## plugins_stop ######################\n");
	plugins_stop();


	printf("\n############## plugins_uninstall #################\n");
	plugins_uninstall();

	printf("plugins_size = %d\n", plugins_size());

	return 0;
err:
	plugins_uninstall();
	return -1;
}
