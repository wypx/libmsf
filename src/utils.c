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
    void* s = NULL;

    DLERROR();/* Clear any existing error */

	/* Writing: cosine = (double (*)(double)) dlsym(handle, "cos");
       would seem more natural, but the C99 standard leaves
       casting from "void *" to a function pointer undefined.
       The assignment used below is the POSIX.1-2003 (Technical
       Corrigendum 1) workaround; see the Rationale for the
       POSIX specification of dlsym(). */
       
    s = DLSYM(handler, symbol);
	
	if (!s) {
		printf("DLSYM() %s\n", DLERROR());
	}
    return s;
}


