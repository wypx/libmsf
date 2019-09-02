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
#include <msf_mem.h>

#include <numa.h>
#include <sched.h>
#include <grp.h>
#include <pwd.h>
#include <sys/sysinfo.h>
#include <sys/resource.h>
#include <sys/utsname.h>

#define MSF_MOD_OS "OS"
#define MSF_OS_LOG(level, ...) \
    msf_log_write(level, MSF_MOD_OS, MSF_FUNC_FILE_LINE, __VA_ARGS__)


#define MB  (1024*1024)

struct msf_meminfo {
    s8      name1[20];
    u64     total;
    s8      name2[20];
    u64     free;
    u32     used_rate;
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
    u64     total;
    u32     used_rate;
};


/* CPU信息描述
 * (user, nice, system,idle, iowait, irq, softirq, stealstolen, guest)的9元组
 *
 * 系统命令 : top命令查看任务管理器实时系统信息
 * 1. CPU使用率: cat /proc/stat 
 *    用户模式(user)
 *    低优先级的用户模式(nice)
 *    内核模式(system)
 *    空闲的处理器时间(idle)
 *    CPU利用率 = 100*(user+nice+system)/(user+nice+system+idle)
 *    第一行: cpu 711 56 2092 7010 104 0 20 0 0 0
 *    上面共有10个值 (单位:jiffies)前面8个值分别为:
 *                        
 *    User time, 711(用户时间)             Nice time, 56 (Nice时间)
 *    System time, 2092(系统时间)          Idle time, 7010(空闲时间)
 *    Waiting time, 104(等待时间)          Hard Irq time, 0(硬中断处理时间)
 *    SoftIRQ time, 20(软中断处理时间)     Steal time, 0(丢失时间)
 *
 *    CPU时间=user+system+nice+idle+iowait+irq+softirq+Stl
 *
 *
 * 2. 内存使用 : cat /proc/meminfo
 *    当前内存的使用量(cmem)以及内存总量(amem)
 *    内存使用百分比 = 100 * (cmem / umem) 
 * 3. 网络负载 : cat /proc/net/dev
 *    从本机输出的数据包数,流入本机的数据包数
 *    平均网络负载 = (输出的数据包+流入的数据包)/2
 */
    
#define CPU_KEY_LEN             16
#define MOD_NAME_CPU            "CPU"
    
    /* Default collection time: every 1 second (0 nanoseconds) */
#define DEFAULT_INTERVAL_SEC    1
#define DEFAULT_INTERVAL_NSEC   0
#define CPU_SNAP_ACTIVE_A       0
#define CPU_SNAP_ACTIVE_B       1

struct cpu_key {
    u32 length;
    s8  name[CPU_KEY_LEN];
};

struct cpu_snapshot {
    s8  cpuid[20];
    u64 user;
    u64 nice;
    u64 system;
    u64 idle;
    u64 iowait;

    /* percent values */
    u64 irq;          /* Overall CPU usage */
    u64 softirq;      /* user space (user + nice) */
    u64 stealstolen;  /* kernel space percent     */
    u64 guest;

    u64 avg_all;
    u64 avg_user;
    u64 avg_kernel;
    /* necessary... */
    struct cpu_key k_cpu;
    struct cpu_key k_user;
    struct cpu_key k_system;
};

struct cpu_stats {
    u32 snap_active;
    s32 interval_sec;           /* interval collection time (Second) */
    s32 interval_nsec;          /* interval collection time (Nanosecond) */
    /* CPU snapshots, we always keep two snapshots */
    struct cpu_snapshot *snap_a;
    struct cpu_snapshot *snap_b;
};

struct msf_os {
    u8      sysname[64];
    u8      nodename[64];
    u8      release[64];
    u8      version[64];
    u8      machine[64];
    u8      domainname[_UTSNAME_DOMAIN_LENGTH];

    /*
     * 如果能知道CPU cache行的大小,那么就可以有针对性地设置内存的对齐值,
     * 这样可以提高程序的效率.有分配内存池的接口, Nginx会将内存池边界
     * 对齐到 CPU cache行大小32位平台, cacheline_size=32 */
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
    u32     tickspersec; /* CPU ticks (Kernel setting) */
    u32     maxhostname;
    u32     maxloginname;

    struct cpu_stats cpu;
    struct msf_meminfo meminfo;
    struct msf_mem  mem;
    struct msf_hdd  hdd;
    s32     en_numa;
    u32     numacnt;
};

s32 process_pin_to_cpu(s32 cpu_id);
s32 thread_pin_to_cpu(u32 cpu_id);
s32 msf_set_nice(s32 increment);
s32 msf_get_priority(s32 which, s32 who);
s32 msf_set_priority(s32 which, s32 who, s32 priority);
u32 msf_get_current_cpu(void);

s32 msf_get_meminfo(void);
s32 msf_get_hdinfo(void);
s32 msf_set_user(struct process *proc);

s32 msf_get_cpuinfo(void);
s32 msf_cpu_perf_init();
s32 msf_cpu_perf_collect(void);

s32 msf_os_init(void);
struct msf_os *msf_get_os(void);
#endif
