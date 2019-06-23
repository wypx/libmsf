/**************************************************************************
*
* Copyright (c) 2017-2019, luotang.me <wypx520@gmail.com>, China.
* All rights reserved.
*
* Distributed under the terms of the GNU General Public License v2.
*
* This software is provided 'as is' with no explicit or implied warranties
* in respect of its properties, including, but not limited to, correctness
* and/or fitness for purpose.
*
**************************************************************************/
#ifndef __MSF_OS_H__
#define __MSF_OS_H__

#include <msf_file.h>
#include <msf_process.h>

#include <sched.h>
#include <grp.h>
#include <pwd.h>
#include <sys/sysinfo.h>
#include <sys/resource.h>
#include <sys/utsname.h>

#include <msf_mem.h>

extern u32 msf_cacheline_size;

#define MB  (1024*1024)

struct msf_meminfo {
    s8      name1[20];
    double  total;
    s8      name2[20];
    double  free;
    double  used_rate;
};

/** Memory information structure */
struct msf_mem {
    u32 total_size; /** total size of memory pool */
    u32 free;        /** free memory */
    u32 used;       /** allocated size */
    u32 real_used;  /** used size plus overhead from malloc */
    u32 max_used;   /** maximum used size since server start? */
    u32 min_frag;   /** minimum number of fragmentations? */
    u32 total_frags; /** number of total memory fragments */
};

struct msf_hdd {
    double total;
    double used_rate;
};

struct msf_os {
    u8      sysname[64];
    u8      nodename[64];
    u8      release[64];
    u8      version[64];
    u8      machine[64];
    u8      domainname[_UTSNAME_DOMAIN_LENGTH];
    
    u32     cacheline_size;
    /* 返回一个分页的大小,单位为字节(Byte).
     * 该值为系统的分页大小,不一定会和硬件分页大小相同*/
    u32     pagesize;
    /* pagesize为4M, pagesize_shift应该为12 
     * pagesize进行移位的次数, 见for (n = ngx_pagesize; 
     * n >>= 1; ngx_pagesize_shift++) {  }
     */
    u32     pagesize_shift;
    u32     pagenum_all;
    u32     pagenum_ava;
    u64     memsize;
    u32     cpuconf;
    u32     cpuonline;
    u32     maxfileopen;
    s32     maxsocket;
    u32     tickspersec;
    u32     maxhostname;
    u32     maxloginname;

    struct msf_meminfo meminfo;
    struct msf_mem  mem;
    struct msf_hdd  hdd;
    s32     en_numa;
    u32     numacnt;
};

s32 msf_get_meminfo(struct msf_meminfo *mem);
s32 msf_get_hdinfo(struct msf_hdd *hd);

void msf_cpuinfo(void);
s32 msf_set_user(struct process *proc);
s32 msf_os_init(void);

extern struct msf_os *g_os;

#endif
