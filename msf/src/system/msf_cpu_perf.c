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

static struct cpu_stats *g_cpu = NULL;
static struct msf_os *g_os = NULL;

MSF_LIBRARY_INITIALIZER(msf_cpu_init, 101) {
    g_os = msf_get_os();
    g_cpu = &(g_os->cpu);
}

#if (( __i386__ || __amd64__ ) && ( __GNUC__ || __INTEL_COMPILER ))
#if ( __i386__ )
static inline void msf_cpuid(u32 i, u32 *buf) {
    /*
     * we could not use %ebx as output parameter if gcc builds PIC,
     * and we could not save %ebx on stack, because %esp is used,
     * when the -fomit-frame-pointer optimization is specified.
     */
    __asm__ (
    "    mov    %%ebx, %%esi;  "

    "    cpuid;                "
    "    mov    %%eax, (%1);   "
    "    mov    %%ebx, 4(%1);  "
    "    mov    %%edx, 8(%1);  "
    "    mov    %%ecx, 12(%1); "

    "    mov    %%esi, %%ebx;  "

    : : "a" (i), "D" (buf) : "ecx", "edx", "esi", "memory" );
}
#else /* __amd64__ */
static inline void msf_cpuid(u32 i, u32 *buf)
{
    u32  eax, ebx, ecx, edx;

    __asm__ ("cpuid" : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (i) );

    buf[0] = eax;
    buf[1] = ebx;
    buf[2] = edx;
    buf[3] = ecx;
}
#endif


/* auto detect the L2 cache line size of modern and widespread CPUs
 * 这个函数便是在获取CPU的信息,根据CPU的型号对ngx_cacheline_size进行设置*/
s32 msf_get_cpuinfo(void)
{
    u8  *vendor;
    u32 vbuf[5], cpu[4], model;

    msf_memzero(vbuf, sizeof(vbuf));

    msf_cpuid(0, vbuf);

    vendor = (u8 *) &vbuf[1];

    if (vbuf[0] == 0) {
        MSF_OS_LOG(DBG_ERROR, "Get cpu info failed.");
        return -1;
    }

    msf_cpuid(1, cpu);

    if (msf_strcmp(vendor, "GenuineIntel") == 0) {
        switch ((cpu[0] & 0xf00) >> 8) {
            /* Pentium */
            case 5:
                MSF_OS_LOG(DBG_DEBUG, "This cpu is Pentium.");
                g_os->cacheline_size = 32;
                break;
            /* Pentium Pro, II, III */
            case 6:
                MSF_OS_LOG(DBG_DEBUG, "This cpu is Pentium Pro.");
                g_os->cacheline_size = 32;

                model = ((cpu[0] & 0xf0000) >> 8) | (cpu[0] & 0xf0);
                if (model >= 0xd0) {
                    /* Intel Core, Core 2, Atom */
                    g_os->cacheline_size = 64;
                }
                break;
            /*
             * Pentium 4, although its cache line size is 64 bytes,
             * it prefetches up to two cache lines during memory read
             */
            case 15:
                MSF_OS_LOG(DBG_DEBUG, "This cpu is Pentium 4.");
                g_os->cacheline_size = 128;
                break;
            default:
                MSF_OS_LOG(DBG_DEBUG, "This cpu is Default.");
                g_os->cacheline_size = 32;
                break;
        }

    } else if (msf_strcmp(vendor, "AuthenticAMD") == 0) {
        MSF_OS_LOG(DBG_DEBUG, "This cpu is AMD.");
        g_os->cacheline_size = 64;
    }
    return 0;
}
#else
s32 msf_cpuinfo(void) {
    MSF_OS_LOG(DBG_DEBUG, "Get cpu info not support.");
    g_os->cacheline_size = 32;
    return 0;
}
#endif


/*********************************CPU Performance**************************************/
/*
 * This routine calculate the average CPU utilization of the system, it
 * takes in consideration the number CPU cores, so it return a value
 * between 0 and 100 based on 'capacity'.
 */
static inline double MSF_CPU_METRIC_SYS_AVERAGE(u32 pre,
            u32 now, struct cpu_stats *cstat)
{
    double diff;
    double total = 0;

    if (pre == now) {
        return 0.0;
    }

    diff = MSF_ABS((s32)(now - pre));
    total = (((diff / g_os->tickspersec) * 100) / g_os->cpuonline) / cstat->interval_sec;

    return total;
}

/* Returns the CPU % utilization of a given CPU core */
static inline double MSF_CPU_METRIC_USAGE(u32 pre, u32 now,
                                      struct cpu_stats *cstat) {
    double diff;
    double total = 0;

    if (pre == now) {
        return 0.0;
    }

    diff = MSF_ABS((s32)(now - pre));
    total = ((diff * 100) / g_os->tickspersec) / cstat->interval_sec;
    return total;
}

static void msf_cpu_snapshots_debug(struct cpu_snapshot *v_snap)
{
    u32 i;
    struct cpu_snapshot *snap;

    for (i = 0; i < g_os->cpuonline + 1; i++) {

        snap = &v_snap[i];
        MSF_OS_LOG(DBG_DEBUG, "##################################");
        MSF_OS_LOG(DBG_DEBUG, "cpuid:%s",       snap->cpuid);
        MSF_OS_LOG(DBG_DEBUG, "user:%lu",       snap->user);
        MSF_OS_LOG(DBG_DEBUG, "nice:%lu",       snap->nice);
        MSF_OS_LOG(DBG_DEBUG, "system:%lu",     snap->system);
        MSF_OS_LOG(DBG_DEBUG, "idle:%lu",       snap->idle);
        MSF_OS_LOG(DBG_DEBUG, "iowait:%lu",     snap->iowait);

        MSF_OS_LOG(DBG_DEBUG, "irq:%lu",        snap->irq);
        MSF_OS_LOG(DBG_DEBUG, "softirq:%lu",    snap->softirq);
        MSF_OS_LOG(DBG_DEBUG, "stealstolen:%lu",snap->stealstolen);
        MSF_OS_LOG(DBG_DEBUG, "guest:%lu",      snap->guest);
        MSF_OS_LOG(DBG_DEBUG, "avg_all:%lu",    snap->avg_all);
        MSF_OS_LOG(DBG_DEBUG, "avg_user:%lu",   snap->avg_user);
        MSF_OS_LOG(DBG_DEBUG, "avg_kernel:%lu", snap->avg_kernel);
        MSF_OS_LOG(DBG_DEBUG, "##################################");
    }
}

inline void msf_cpu_snapshots_switch(struct cpu_stats *cstats)
{
    if (cstats->snap_active == CPU_SNAP_ACTIVE_A) {
        cstats->snap_active = CPU_SNAP_ACTIVE_B;
        //msf_cpu_snapshots_debug(cstats->snap_b);
    }
    else {
        cstats->snap_active = CPU_SNAP_ACTIVE_A;
        //msf_cpu_snapshots_debug(cstats->snap_a);
    }
}

/*
 * Given the two snapshots, calculate the % used in user and kernel space,
 * it returns the active snapshot.
 */
struct cpu_snapshot* msf_cpu_snapshot_percent(struct cpu_stats *cstats)
{
    u32 i;
    u32 sum_pre;
    u32 sum_now;
    struct cpu_snapshot *arr_pre = NULL;
    struct cpu_snapshot *arr_now = NULL;
    struct cpu_snapshot *snap_pre = NULL;
    struct cpu_snapshot *snap_now = NULL;

    if (cstats->snap_active == CPU_SNAP_ACTIVE_A) {
        arr_now = cstats->snap_a;
        arr_pre = cstats->snap_b;
    } else if (cstats->snap_active == CPU_SNAP_ACTIVE_B) {
        arr_now = cstats->snap_b;
        arr_pre = cstats->snap_a;
    }

    for (i = 0; i <= g_os->cpuonline + 1; i++) {
        snap_pre = &arr_pre[i];
        snap_now = &arr_now[i];

        /* Calculate overall CPU usage (user space + kernel space */
        sum_pre = (snap_pre->user + snap_pre->nice + snap_pre->system);
        sum_now = (snap_now->user + snap_now->nice + snap_now->system);

        if (i == 0) {
            snap_now->avg_all = MSF_CPU_METRIC_SYS_AVERAGE(sum_pre, sum_now, cstats);
        } else {
            snap_now->avg_all = MSF_CPU_METRIC_USAGE(sum_pre, sum_now, cstats);
        }

        /* User space CPU% */
        sum_pre = (snap_pre->user + snap_pre->nice);
        sum_now = (snap_now->user + snap_now->nice);
        if (i == 0) {
            snap_now->avg_user = MSF_CPU_METRIC_SYS_AVERAGE(sum_pre, sum_now, cstats);
        } else {
            snap_now->avg_user = MSF_CPU_METRIC_USAGE(sum_pre, sum_now, cstats);
        }

        /* Kernel space CPU% */
        if (i == 0) {
            snap_now->avg_kernel = MSF_CPU_METRIC_SYS_AVERAGE(snap_pre->system,
                                                        snap_now->system,
                                                        cstats);
        } else {
            snap_now->avg_kernel = MSF_CPU_METRIC_USAGE(snap_pre->system,
                                                  snap_now->system,
                                                  cstats);
        }

        if (i == 0) {
            MSF_OS_LOG(DBG_DEBUG, "CPU[all] all=%f user=%f system=%f",
                       snap_now->avg_all, snap_now->avg_user,snap_now->avg_kernel);
        } else {
            MSF_OS_LOG(DBG_DEBUG, "CPU[i=%i] all=%f user=%f system=%f",
                      i-1, snap_now->avg_all, snap_now->avg_user, snap_now->avg_kernel);
        }
    }

    return arr_now;
}

/* Retrieve CPU load from the system (through ProcFS) */
static inline s32 msf_cpu_proc_load(struct cpu_stats *cstat)
{
    u32 i;
    s32 rc = -1;
    s8  line[255];
    s32 len = 0;
    s8 *fmt = NULL;
    FILE *fp = NULL;
    struct cpu_snapshot *s;
    struct cpu_snapshot *snap_arr;

    fp = fopen("/proc/stat", "r");
    if (!fp) {
        MSF_OS_LOG(DBG_ERROR,
            "Failed to open proc stat file, errno(%d).", errno);
        return -1;
    }

    if (CPU_SNAP_ACTIVE_A == cstat->snap_active) {
        snap_arr = cstat->snap_a;
    } else {
        snap_arr = cstat->snap_b;
    }

    msf_memzero(snap_arr, sizeof(struct cpu_snapshot)*(g_os->cpuonline+1));
    /* Always read (n_cpus + 1) lines */
    for (i = 0; i < g_os->cpuonline + 1; i++) {
        msf_memzero(line, sizeof(line));
        if (fgets(line, sizeof(line) - 1, fp)) {
            len = msf_strlen(line);
            if (line[len - 1] == '\n') {
                line[--len] = 0;
                if (len && line[len - 1] == '\r') {
                    line[--len] = 0;
                }
            }

            s = &snap_arr[i];
            if (i == 0) {
                fmt = "%s  %lu %lu %lu %lu %lu";
                rc = sscanf(line,
                             fmt,
                             s->cpuid,
                             &s->user,
                             &s->nice,
                             &s->system,
                             &s->idle,
                             &s->iowait);
                if (rc < 6) {
                    MSF_OS_LOG(DBG_ERROR,
                        "Failed to format cpu data, errno(%d).", errno);
                    sfclose(fp);
                    return -1;
                }
            } else {
                fmt = "%s %lu %lu %lu %lu %lu %lu %lu %lu %lu";
                rc = sscanf(line, fmt, s->cpuid, &s->user, &s->nice,
                            &s->system, &s->idle, &s->iowait,
                            &s->irq, &s->softirq, &s->stealstolen, &s->guest);
                if (rc <= 9) {
                    MSF_OS_LOG(DBG_ERROR,
                        "Failed to format cpu%d data, errno(%d).", i, errno);
                    sfclose(fp);
                    return -1;
                }
            }
        }
        else {
            break;
        }
    }

    sfclose(fp);
    return 0;
}

/* Init CPU input */
s32 msf_cpu_perf_init(void)
{
    s32 rc = -1;
    s8  *pval = NULL;

    /* Collection time setting */
    g_cpu->interval_sec   = DEFAULT_INTERVAL_SEC;
    g_cpu->interval_nsec  = DEFAULT_INTERVAL_NSEC;

    /* Initialize buffers for CPU stats */
    g_cpu->snap_a = MSF_NEW(struct cpu_snapshot, g_os->cpuonline+ 1);
    if (!g_cpu->snap_a) {
        MSF_OS_LOG(DBG_ERROR, "Malloc cpu_snapshot_a error.");
        return -1;
    }

    g_cpu->snap_b = MSF_NEW(struct cpu_snapshot, g_os->cpuonline+ 1);
    if (!g_cpu->snap_b) {
        MSF_OS_LOG(DBG_ERROR, "Malloc cpu_snapshot_b error.");
        sfree(g_cpu->snap_a);
        return -1;
    }
    g_cpu->snap_active = CPU_SNAP_ACTIVE_A;

    /* Get CPU load, ready to be updated once fired the calc callback */
    rc = msf_cpu_proc_load(g_cpu);
    if (rc != 0) {
        MSF_OS_LOG(DBG_ERROR, "Could not obtain CPU data.");
        sfree(g_cpu->snap_b);
        sfree(g_cpu->snap_a);
        return -1;
    }

    g_cpu->snap_active = CPU_SNAP_ACTIVE_B;

    /* Set our collector based on time, CPU usage every 1 second */

    return 0;
}


/* Callback to gather CPU usage between now and previous snapshot */
s32 msf_cpu_perf_collect(void)
{
    u32 i;
    s32 rc  = -1;
    struct cpu_snapshot* s;

    /* Get the current CPU usage */
    rc = msf_cpu_proc_load(g_cpu);
    if (rc != 0) {
        return -1;
    }

    s = msf_cpu_snapshot_percent(g_cpu);

    for (i = 1; i < g_os->cpuonline + 1; i++) {
        struct cpu_snapshot *e = &s[i];
    }

    msf_cpu_snapshots_switch(g_cpu);

    return 0;
}

