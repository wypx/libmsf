
#include <msf_os.h>

s32  msf_ncpu; //cpu个数
s32  msf_max_sockets;//每个进程能打开的最多文件数
u32  msf_inherited_nonblocking;
u32  msf_tcp_nodelay_and_tcp_nopush;


/* 每个进程能打开的最多文件数 */
struct rlimit  rlmt;

/* 返回一个分页的大小，单位为字节(Byte).
 * 该值为系统的分页大小,不一定会和硬件分页大小相同*/
u32  msf_pagesize;

/* pagesize为4M, pagesize_shift应该为12 
 * pagesize进行移位的次数, 见for (n = ngx_pagesize; 
 * n >>= 1; ngx_pagesize_shift++) {  }
 */
u32  msf_pagesize_shift;

/*
 * 如果能知道CPU cache行的大小,那么就可以有针对性地设置内存的对齐值,
 * 这样可以提高程序的效率.有分配内存池的接口, Nginx会将内存池边界
 * 对齐到 CPU cache行大小32位平台, cacheline_size=32 */
u32 msf_cacheline_size;


u8  msf_linux_kern_ostype[50];
u8  msf_linux_kern_osrelease[50];


int msf_osinfo() {

    struct utsname	u;   
    if (uname(&u) == -1) {
       return -1;
    }

    printf("u.sysname:%s\n", u.sysname); //当前操作系统名
    printf("u.nodename:%s\n", u.nodename); //网络上的名称
    printf("u.release:%s\n", u.release); //当前发布级别
    printf("u.version:%s\n", u.version); //当前发布版本
    printf("u.machine:%s\n", u.machine); //当前硬件体系类型
    //printf("u.__domainname:%s\n", u.__domainname); //当前硬件体系类型

#if _UTSNAME_DOMAIN_LENGTH - 0
#ifdef __USE_GNU
    //printf("u.domainname::%s\n ", u.domainname);
    //char domainname[_UTSNAME_DOMAIN_LENGTH]; //当前域名
#else
    printf("u.__domainname::%s\n", u.__domainname);
    //char __domainname[_UTSNAME_DOMAIN_LENGTH];
#endif
#endif
    return 0;
}

s32 msf_set_user(struct process *proc) {

    s8               *group = NULL;
    struct passwd    *pwd = NULL;
    struct group     *grp = NULL;

     if (proc->user != (uid_t) MSF_CONF_UNSET_UINT) {
         printf("is duplicate\n");
         return 0;
    }

    if (geteuid() != 0) {
        printf(
           "the \"user\" directive makes sense only "
           "if the master process runs "
           "with super-user privileges, ignored\n");
        return 0;
    }

    pwd = getpwnam((const s8 *) proc->username);
    if (pwd == NULL) {
        return -1;
    }

   	proc->user = pwd->pw_uid;

    grp = getgrnam(group);
    if (grp == NULL) {      
        return -1;
    }

    proc->group = grp->gr_gid;

    return 0;

}

#if (MSF_HAVE_CPUSET_SETAFFINITY)

#include <sys/cpuset.h>

void msf_setaffinity(u64 cpu_affinity) {
    cpuset_t    mask;
    u32  i;

    printf("cpuset_setaffinity(0x%08Xl)\n", cpu_affinity);

    CPU_ZERO(&mask);
    i = 0;
    do {
        if (cpu_affinity & 1) {
            CPU_SET(i, &mask);
        }
        i++;
        cpu_affinity >>= 1;
    } while (cpu_affinity);

    if (cpuset_setaffinity(CPU_LEVEL_WHICH, CPU_WHICH_PID, -1,
                           sizeof(cpuset_t), &mask) == -1)
    {

    }
}

#elif (MSF_HAVE_SCHED_SETAFFINITY)

void msf_setaffinity(u64 cpu_affinity)
{
    cpu_set_t   mask;
    u32  i;

    printf("sched_setaffinity(%ld)\n", cpu_affinity);

    CPU_ZERO(&mask);
    i = 0;
    do {
        if (cpu_affinity & 1) {
            CPU_SET(i, &mask);
        }
        i++;
        cpu_affinity >>= 1;
    } while (cpu_affinity);

    if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1) {
    }
}

#endif


s32 msf_set_priority(struct process *proc) {

    setpriority(PRIO_PROCESS, 0, proc->priority);

    return 0;
}

s32 msf_set_rlimit(struct process *proc) {

    struct rlimit     rlmt;

    if (proc->rlimit_nofile != -1) {
        rlmt.rlim_cur = (rlim_t) proc->rlimit_nofile;
        rlmt.rlim_max = (rlim_t) proc->rlimit_nofile;

        //RLIMIT_NOFILE指定此进程可打开的最大文件描述词大一的值，超出此值，将会产生EMFILE错误。
        if (setrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
        }
    }

    if (proc->rlimit_core != -1) {
        rlmt.rlim_cur = (rlim_t) proc->rlimit_core;
        rlmt.rlim_max = (rlim_t) proc->rlimit_core;
        //修改工作进程的core文件尺寸的最大值限制(RLIMIT_CORE)，用于在不重启主进程的情况下增大该限制。
        if (setrlimit(RLIMIT_CORE, &rlmt) == -1) {
        }

	}


    if (geteuid() == 0) {
        if (setgid(proc->group) == -1) {
            /* fatal */
            exit(2);
        }

        if (initgroups(proc->username, proc->group) == -1) {
        }

        if (setuid(proc->user) == -1) {
            /* fatal */
            exit(2);
        }
    }


    msf_setaffinity(proc->cpu_affinity);

    /* allow coredump after setuid() in Linux 2.4.x */
    msf_enable_coredump();

    return 0;
}


/**
 * RAM
 */
s32 msf_get_meminfo(struct msf_meminfo *mem) {

    FILE *fp = NULL;                      
    s8 buff[256];
                                                                                                         
    fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        return -1;
    }

    msf_memzero(buff, sizeof(buff));
    fgets(buff, sizeof(buff), fp); 
    sscanf(buff, "%s %le %s*", mem->name1, &mem->total, mem->name2); 

    msf_memzero(buff, sizeof(buff));
    fgets(buff, sizeof(buff), fp);
    sscanf(buff, "%s %le %s*", mem->name1, &mem->free, mem->name2); 

    mem->used_rate= (1 - mem->total/ mem->total) * 100;

    sfclose(fp);

    return 0;
}

s32 msf_get_hdinfo(struct msf_hdinfo *hd) {

    FILE *fp = NULL;
    s8 buffer[80],a[80],d[80],e[80],f[80], buf[256];
    double c,b;
    double dev_total = 0, dev_used = 0;

    fp = popen("df", "r");
    if (!fp) {
        return -1;
    }

    fgets(buf, sizeof(buf), fp);
    while (6 == fscanf(fp, "%s %lf %lf %s %s %s", a, &b, &c, d, e, f)) {
        dev_total += b;
        dev_used += c;
    }

    hd->total = dev_total / (1024*1024);
    hd->used_rate = dev_used/ dev_total * 100;

    pclose(fp);

    return 0;
}

s32 msf_os_init(void) {

    u32  n;

    struct utsname  u;
    
    if (uname(&u) == -1) {
        return -1;
    }

    (void) memcpy(msf_linux_kern_ostype, (u8 *) u.sysname,
                       sizeof(msf_linux_kern_ostype));

    (void) memcpy(msf_linux_kern_osrelease, (u8 *) u.release,
                       sizeof(msf_linux_kern_osrelease));

    msf_pagesize = getpagesize();

    for (n = msf_pagesize; n >>= 1; msf_pagesize_shift++) { /* void */ }

    if (msf_ncpu == 0) {
        msf_ncpu = sysconf(_SC_NPROCESSORS_ONLN); //获取系统中可用的 CPU 数量, 没有被激活的 CPU 则不统计 在内, 例如热添加后还没有激活的. 
    }


    if (msf_ncpu < 1) {
        msf_ncpu = 1;
    }

    msf_cpuinfo();

    if (getrlimit(RLIMIT_NOFILE, &rlmt) == -1) {  
          return -1;
    }

    msf_max_sockets = (s32) rlmt.rlim_cur;

    return 0;
}
