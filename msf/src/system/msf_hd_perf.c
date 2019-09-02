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

struct msf_hdd *g_hdd = NULL;

MSF_LIBRARY_INITIALIZER(msf_hdd_init, 101) {
    g_hdd = &(msf_get_os()->hdd);
}

s32 msf_get_hdinfo(void) {

    FILE *fp = NULL;
    s8 buffer[80],a[80],d[80],e[80],f[80], buf[256];
    double c,b;
    double dev_total = 0, dev_used = 0;

    fp = popen("df", "r");
    if (!fp) {
        return -1;
    }

    fgets(buf, sizeof(buf), fp);
    while (6 == fscanf(fp, "%s %lf %lf %s %s %s",
        a, &b, &c, d, e, f)) {
        dev_total += b;
        dev_used += c;
    }

    g_hdd->total = dev_total / MB;
    g_hdd->used_rate = dev_used / dev_total * 100;

    pclose(fp);

    return 0;
}

