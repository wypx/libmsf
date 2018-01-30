
#include "utils.h"

void* plugin_load_dynamic(const char* path)
{
    void* handle = NULL;

    handle = DLOPEN_L(path);
    if (!handle) {
        printf("DLOPEN_L() %s\n", DLERROR());
    }

    return handle;
}

void* plugin_load_symbol(void* handler, const char* symbol)
{
    void *s = NULL;

    DLERROR();
    s = DLSYM(handler, symbol);
	
	if (!s) {
		printf("DLSYM() %s\n", DLERROR());
	}
    return s;
}


