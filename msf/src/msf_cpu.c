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

#include <msf_os.h>
#include <msf_cpu.h>
#include <msf_svc.h>
#include <msf_thread.h>

#if (( __i386__ || __amd64__ ) && ( __GNUC__ || __INTEL_COMPILER ))

static inline void msf_cpuid(u32 i, u32 *buf);


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

    __asm__ (

        "cpuid"

    : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (i) );

    buf[0] = eax;
    buf[1] = ebx;
    buf[2] = edx;
    buf[3] = ecx;
}


#endif


/* auto detect the L2 cache line size of modern and widespread CPUs
 * è¿™ä¸ªå‡½æ•°ä¾¿æ˜¯åœ¨èŽ·å–CPUçš„ä¿¡æ¯ï¼Œæ ¹æ®CPUçš„åž‹å·å¯¹ngx_cacheline_sizeè¿›è¡Œè®¾ç½®*/
void msf_cpuinfo(void)
{
    u8    *vendor;
    u32   vbuf[5], cpu[4], model;

    vbuf[0] = 0;
    vbuf[1] = 0;
    vbuf[2] = 0;
    vbuf[3] = 0;
    vbuf[4] = 0;

    msf_cpuid(0, vbuf);

    vendor = (u8 *) &vbuf[1];

    if (vbuf[0] == 0) {
        return;
    }

    msf_cpuid(1, cpu);

    if (msf_strcmp(vendor, "GenuineIntel") == 0) {

        switch ((cpu[0] & 0xf00) >> 8) {

        /* Pentium */
        case 5:
            msf_cacheline_size = 32;
            break;

        /* Pentium Pro, II, III */
        case 6:
            msf_cacheline_size = 32;

            model = ((cpu[0] & 0xf0000) >> 8) | (cpu[0] & 0xf0);

            if (model >= 0xd0) {
                /* Intel Core, Core 2, Atom */
                msf_cacheline_size = 64;
            }

            break;

        /*
         * Pentium 4, although its cache line size is 64 bytes,
         * it prefetches up to two cache lines during memory read
         */
        case 15:
            msf_cacheline_size = 128;
            break;
        default:
            msf_cacheline_size = 32;
            break;
        }

    } else if (msf_strcmp(vendor, "AuthenticAMD") == 0) {
        msf_cacheline_size = 64;
    }
}
#else
void msf_cpuinfo(void) {
    /*stub function*/
}
#endif

#define CPU_KEY_FORMAT(s, key, i)                                \
    s->k_##key.length = snprintf(s->k_##key.name,                \
                                 CPU_KEY_LEN,                    \
                                 "cpu%i.%s", i - 1, #key)

/*
 * This routine calculate the average CPU utilization of the system, it
 * takes in consideration the number CPU cores, so it return a value
 * between 0 and 100 based on 'capacity'.
 */
static inline double CPU_METRIC_SYS_AVERAGE(unsigned long pre, unsigned long now,
                                            struct cpu_config *ctx)
{
    double diff;
    double total = 0;

    if (pre == now) {
        return 0.0;
    }

    diff = abs(now - pre);
    total = (((diff / ctx->cpu_ticks) * 100) / ctx->n_processors) / ctx->interval_sec;

    return total;
}

/* Returns the CPU % utilization of a given CPU core */
static inline double CPU_METRIC_USAGE(unsigned long pre, unsigned long now,
                                      struct cpu_config *ctx) {
    double diff;
    double total = 0;

    if (pre == now) {
        return 0.0;
    }

    diff = MSF_ABS((s32)(now - pre));
    total = ((diff * 100) / ctx->cpu_ticks) / ctx->interval_sec;
    return total;
}


static inline void snapshot_key_format(s32 cpus, struct cpu_snapshot *snap_arr) {
    s32 i;
    struct cpu_snapshot *snap;

    snap = &snap_arr[0];
    strncpy(snap->k_cpu.name, "cpu", 3);
    snap->k_cpu.name[3] = '\0';

    for (i = 1; i <= cpus; i++) {
        snap = (struct cpu_snapshot *) &snap_arr[i];
        CPU_KEY_FORMAT(snap, cpu, i);
        CPU_KEY_FORMAT(snap, user, i);
        CPU_KEY_FORMAT(snap, system, i);
    }
}

static inline void cpu_snapshots_switch(struct cpu_stats *cstats)
{
    if (cstats->snap_active == CPU_SNAP_ACTIVE_A) {
        cstats->snap_active = CPU_SNAP_ACTIVE_B;
    }
    else {
        cstats->snap_active = CPU_SNAP_ACTIVE_A;
    }
}

/*
 * Given the two snapshots, calculate the % used in user and kernel space,
 * it returns the active snapshot.
 */
struct cpu_snapshot* cpu_snapshot_percent(struct cpu_stats *cstats,
                                      struct cpu_config *ctx)
{
    s32 i;
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

    for (i = 0; i <= ctx->n_processors; i++) {
        snap_pre = &arr_pre[i];
        snap_now = &arr_now[i];

        /* Calculate overall CPU usage (user space + kernel space */
        sum_pre = (snap_pre->user + snap_pre->nice + snap_pre->system);
        sum_now = (snap_now->user + snap_now->nice + snap_now->system);

        if (i == 0) {
            snap_now->avg_all = CPU_METRIC_SYS_AVERAGE(sum_pre, sum_now, ctx);
        } else {
            snap_now->avg_all = CPU_METRIC_USAGE(sum_pre, sum_now, ctx);
        }

        /* User space CPU% */
        sum_pre = (snap_pre->user + snap_pre->nice);
        sum_now = (snap_now->user + snap_now->nice);
        if (i == 0) {
            snap_now->avg_user = CPU_METRIC_SYS_AVERAGE(sum_pre, sum_now, ctx);
        } else {
            snap_now->avg_user = CPU_METRIC_USAGE(sum_pre, sum_now, ctx);
        }

        /* Kernel space CPU% */
        if (i == 0) {
            snap_now->avg_kernel = CPU_METRIC_SYS_AVERAGE(snap_pre->system,
                                                        snap_now->system,
                                                        ctx);
        } else {
            snap_now->avg_kernel = CPU_METRIC_USAGE(snap_pre->system,
                                                  snap_now->system,
                                                  ctx);
        }

        if (i == 0) {
            printf("cpu[all] all=%f user=%f system=%f",
                       snap_now->avg_all, snap_now->avg_user,snap_now->avg_kernel);
        } else {
           printf("cpu[i=%i] all=%f user=%f system=%f",
                      i-1, snap_now->avg_all, snap_now->avg_user, snap_now->avg_kernel);
        }
    }

    return arr_now;
}

/* Retrieve CPU load from the system (through ProcFS) */
static inline double cpu_proc_load(struct cpu_config *ctx)
{
    s32 i;
    s32 rc = -1;
    s8 	line[255];
    s32 len = 0;
    s8 *fmt = NULL;
    FILE *fp = NULL;
    struct cpu_snapshot *s;
    struct cpu_snapshot *snap_arr;

    fp = fopen("/proc/stat", "r");
    if (!fp) {
        return -1;
    }

    if (CPU_SNAP_ACTIVE_A == ctx->cstats.snap_active) {
        snap_arr = ctx->cstats.snap_a;
    }
    else {
        snap_arr = ctx->cstats.snap_b;
    }

    /* Always read (n_cpus + 1) lines */
    for (i = 0; i <= ctx->processors_avalible; i++) {
        if (fgets(line, sizeof(line) - 1, fp)) {
            len = strlen(line);
            if (line[len - 1] == '\n') {
                line[--len] = 0;
                if (len && line[len - 1] == '\r') {
                    line[--len] = 0;
                }
            }

            s = &snap_arr[i];
            if (i == 0) {
                fmt = " cpu  %lu %lu %lu %lu %lu";
                rc = sscanf(line,
                             fmt,
                             &s->user,
                             &s->nice,
                             &s->system,
                             &s->idle,
                             &s->iowait);
                if (rc < 5) {
                    sfclose(fp);
                    return -1;
                }
            } else {
                fmt = " %s %lu %lu %lu %lu %lu %lu %lu %lu %lu";
                rc = sscanf(line, fmt, &s->cpuid, &s->user, &s->nice,
                      		&s->system, &s->idle, &s->iowait,
                            &s->irq, &s->softirq, &s->stealstolen, &s->guest);
                if (rc <= 9) {
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
static int cpu_init()
{
    s32	rc =-1;
    s8	*pval = NULL;
    struct cpu_config *ctx = NULL;

    /* Allocate space for the configuration */
    ctx = MSF_NEW(struct cpu_config, 1);
    if (!ctx) {
        printf("calloc tm_cpu_config error\n");
        return -1;
    }

    /* Gather number of processors and CPU ticks linux2.6 Later
     refer:http://blog.csdn.net/u012317833/article/details/39476831 */

    ctx->processors_configured  = sysconf(_SC_NPROCESSORS_CONF);
    ctx->processors_avalible    = sysconf(_SC_NPROCESSORS_ONLN);
    ctx->total_pages            = sysconf(_SC_PHYS_PAGES);
    ctx->free_pages             = sysconf(_SC_AVPHYS_PAGES);
    ctx->page_size              = sysconf(_SC_PAGESIZE);
    //ctx->total_mem            = (ctx->total_pages * ctx->page_size) / ONE_MB;
    //ctx->free_mem             = (ctx->free_pages  * ctx->page_size) / ONE_MB; 
    ctx->cpu_ticks              = sysconf(_SC_CLK_TCK);
    ctx->total_fds              = sysconf(_SC_OPEN_MAX); 

    /* Collection time setting */
    ctx->interval_sec           = DEFAULT_INTERVAL_SEC;
    ctx->interval_nsec          = DEFAULT_INTERVAL_NSEC;

    /* Initialize buffers for CPU stats */
    ctx->cstats.snap_a = MSF_NEW(struct cpu_snapshot, ctx->processors_avalible+ 1);
    if (!(ctx->cstats.snap_a)) {
        printf("malloc cpu_snapshot a  error\n");
        sfree(ctx);
        return -1;
    }

    ctx->cstats.snap_b = MSF_NEW(struct cpu_snapshot, ctx->processors_avalible+ 1);
    if (!(ctx->cstats.snap_b)) {
        printf("malloc cpu_snapshot b error\n");
        sfree(ctx->cstats.snap_a);
        sfree(ctx);
        return -1;
    }

    /* Initialize each array */
    snapshot_key_format(ctx->processors_avalible, ctx->cstats.snap_a);
    snapshot_key_format(ctx->processors_avalible, ctx->cstats.snap_b);
    ctx->cstats.snap_active = CPU_SNAP_ACTIVE_A;

    /* Get CPU load, ready to be updated once fired the calc callback */
    rc = cpu_proc_load(ctx);
    if (rc != 0) {
        printf("Could not obtain CPU data\n");
        sfree(ctx->cstats.snap_b);
        sfree(ctx->cstats.snap_a);
        sfree(ctx);
        return -1;
    }

    ctx->cstats.snap_active = CPU_SNAP_ACTIVE_B;

    /* Set our collector based on time, CPU usage every 1 second */

    return 0;
}



/* Callback to gather CPU usage between now and previous snapshot */
s32 cpu_collect(struct cpu_config *cpu_ctx)
{
    s32 i;
    s32 rc	= -1;
    struct cpu_config* ctx = cpu_ctx;
    struct cpu_stats* cstats = &ctx->cstats;
    struct cpu_snapshot* s;

    /* Get the current CPU usage */
    rc = cpu_proc_load(ctx);
    if (rc != 0) {
        return -1;
    }

    s = cpu_snapshot_percent(cstats, ctx);

    for (i = 1; i < ctx->n_processors + 1; i++) {
        struct cpu_snapshot *e = &s[i];
    }

    cpu_snapshots_switch(cstats);

    return 0;
}



/*********************************CPU Affinity**************************************/
struct getcpu_cache {
    unsigned long blob[128 / sizeof(long)];
};

typedef long (*vgetcpu_fn)(unsigned *cpu,
              unsigned *node, struct getcpu_cache *tcache);
vgetcpu_fn vgetcpu;

s32 msf_init_vgetcpu(void) {
    void *vdso;

    MSF_DLERROR();
    vdso = MSF_DLOPEN_L("linux-vdso.so.1");
    if (!vdso)
        return -1;
    vgetcpu = (vgetcpu_fn)MSF_DLSYM(vdso, "__vdso_getcpu");
    MSF_DLCLOSE(vdso);
    return !vgetcpu ? -1 : 0;
}

u32 msf_get_cpu(void) {
    static s32 first = 1;
    u32 cpu;

    if (!first && vgetcpu) {
        vgetcpu(&cpu, NULL, NULL);
        return cpu;
    }
    if (!first)
        return sched_getcpu();

    first = 0;
    if (msf_init_vgetcpu() < 0) {
        vgetcpu = NULL;
        return sched_getcpu();
    }
    vgetcpu(&cpu, NULL, NULL);
    return cpu;
}


s32 thread_pin_to_cpu(u32 cpu_id) {
    s32 rc;

    cpu_set_t cpu_info;
    CPU_ZERO(&cpu_info);
    CPU_SET(cpu_id, &cpu_info);
    rc = pthread_setaffinity_np(pthread_self(), 
        sizeof(cpu_set_t), &cpu_info);

    if (rc < 0) {
        printf("set thread(%u) affinity failed\n", cpu_id);
    }

#if 0
    for (s32 j = 0; j < srv->max_cores; j++) {
        if (CPU_ISSET(j, &cpu_info)) {
            printf("CPU %d\n", j);
        }
    }
#endif
    return rc;
}


#if (MSF_HAVE_CPUSET_SETAFFINITY)

#include <sys/cpuset.h>

s32 process_pin_to_cpu(s32 cpu_id)
{
    s32 rc = -1;

    cpu_set_t mask;
    cpu_set_t get;

    CPU_ZERO(&mask);
    CPU_SET(cpu_id, &mask);

    if (numa_num_task_cpus() > CPU_SETSIZE)
        return -1;

    if (CPU_COUNT(&mask) == 1)
        return 0;

    if (cpuset_setaffinity(CPU_LEVEL_WHICH, CPU_WHICH_PID, -1, 
        sizeof(cpuset_t), &mask) < 0) {
        printf("Could not set CPU affinity, continuing.\n");
    }

    /* guaranteed to take effect immediately */
    sched_yield();

    return rc;
}


#elif (MSF_HAVE_SCHED_SETAFFINITY)

s32 process_pin_to_cpu(s32 cpu_id)
{
    s32 rc = -1;

    cpu_set_t mask;
    cpu_set_t get;

    CPU_ZERO(&mask);
    CPU_SET(cpu_id, &mask);

    if (numa_num_task_cpus() > CPU_SETSIZE)
        return -1;

    if (CPU_COUNT(&mask) == 1)
        return 0;
    
    if (sched_setaffinity(0, sizeof(mask), &mask) < 0) {
        printf("Could not set CPU affinity, continuing.\n");
    }

    /* guaranteed to take effect immediately */
    sched_yield();

    CPU_ZERO(&get);
    if (sched_getaffinity(0, sizeof(get), &get) == -1) {
        printf("Could not get CPU affinity, continuing.\n");
    }

    for (int i = 0; i < get_nprocs_conf(); i++) {
        if (CPU_ISSET(i, &get))//ÅÐ¶ÏÏß³ÌÓëÄÄ¸öCPUÓÐÇ×ºÍÁ¦  
        {
            printf("this process %d is running processor: %d\n", i, i);
        }
    }  

    return rc;
}

#endif


s32 msf_set_priority(s32 priority) {

    setpriority(PRIO_PROCESS, 0, priority);

    return 0;
}

