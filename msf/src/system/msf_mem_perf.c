/**************************************************************************
*
* Copyright (c) 2017-2020, luotang.me <wypx520@gmail.com>, China.
* All rights reserved.
*
* Distributed under the terms of the GNU General Public License v2.
*
* This software is provided 'as is' with no explicit or implied warranties
* in respect of its properties, including, but not limited to, correctness
* and/or fitness for purpose.
*
**************************************************************************/
#include <msf_os.h>

static struct msf_os *g_os = NULL;
static struct msf_meminfo *g_mem = NULL;

MSF_LIBRARY_INITIALIZER(msf_mem_perf_init, 101) {
    g_os = msf_get_os();
    g_mem = &(g_os->meminfo);
}


/**
 * RAM
 */
s32 msf_get_meminfo(void) {

    FILE *fp = NULL;
    s8 buff[256];

    fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        MSF_OS_LOG(DBG_DEBUG, "Get cpu info not support.");
        return -1;
    }

    msf_memzero(buff, sizeof(buff));
    fgets(buff, sizeof(buff), fp); 
    sscanf(buff, "%s %lu %s*", g_mem->name1, &g_mem->total, g_mem->name2); 

    msf_memzero(buff, sizeof(buff));
    fgets(buff, sizeof(buff), fp);
    sscanf(buff, "%s %lu %s*", g_mem->name1, &g_mem->free, g_mem->name2); 

    g_mem->used_rate= (1 - g_mem->total/ g_mem->total) * 100;

    sfclose(fp);

    return 0;
}

