#include <msf_utils.h>
#include <sched.h>
#include <numa.h>

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

#define CPU_KEY_LEN     16
#define MOD_NAME_CPU    "CPU"
 
struct cpu_key {
    u32 length;
    s8  name[CPU_KEY_LEN];
};

 struct cpu_snapshot {
    s8	cpuid[20];
    unsigned long user;
    unsigned long nice;
    unsigned long system;
    unsigned long idle;
    unsigned long iowait;

    /* percent values */
    unsigned long irq;          /* Overall CPU usage */
    unsigned long softirq;      /* user space (user + nice) */
    unsigned long stealstolen;  /* kernel space percent     */
    unsigned long guest;

    double avg_all;
    double avg_user;
    double avg_kernel;
    /* necessary... */
    struct cpu_key k_cpu;
    struct cpu_key k_user;
    struct cpu_key k_system;
}__attribute__((__packed__));


/* Default collection time: every 1 second (0 nanoseconds) */
#define DEFAULT_INTERVAL_SEC    1
#define DEFAULT_INTERVAL_NSEC   0
#define CPU_SNAP_ACTIVE_A       0
#define CPU_SNAP_ACTIVE_B       1

#if 0
struct cpu_snapshot {
    /* data snapshots */
    s8  v_cpuid[8];
    u32 v_user;
    u32 v_nice;
    u32 v_system;
    u32 v_idle;
    u32 v_iowait;

    /* percent values */
    double p_cpu;           /* Overall CPU usage        */
    double p_user;          /* user space (user + nice) */
    double p_system;        /* kernel space percent     */

    /* necessary... */
    struct cpu_key k_cpu;
    struct cpu_key k_user;
    struct cpu_key k_system;
};
#endif

struct cpu_stats {
    u32 snap_active;

    /* CPU snapshots, we always keep two snapshots */
    struct cpu_snapshot *snap_a;
    struct cpu_snapshot *snap_b;
};

/* CPU Input configuration & context */
struct cpu_config {
    /* setup */
    long processors_configured;
    long processors_avalible;   /* number of processors currently online (available) */
    long cpu_ticks;             /* CPU ticks (Kernel setting) */
    long total_pages;
    long free_pages;
    long page_size;
    long long total_mem;
    long long free_mem ;
    long total_fds;             /* one process open file fd MAX %ld */

    long n_processors;
    s32 interval_sec;           /* interval collection time (Second) */
    s32 interval_nsec;          /* interval collection time (Nanosecond) */
    struct cpu_stats cstats;
};

u32 msf_get_cpu(void);

s32 thread_pin_to_cpu(u32 cpu_id);
s32 process_pin_to_cpu(s32 cpu_id);


