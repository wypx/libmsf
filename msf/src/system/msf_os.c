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
#include <msf_network.h>

static struct msf_os os;
static struct msf_os *g_os = &os;

s32 msf_set_user(struct process *proc) {

    s8               *group = NULL;
    struct passwd    *pwd = NULL;
    struct group     *grp = NULL;

     if (proc->user != (uid_t) MSF_CONF_UNSET_UINT) {
         MSF_OS_LOG(DBG_DEBUG, "User is duplicate.");
         return 0;
    }

    if (geteuid() != 0) {
        MSF_OS_LOG(DBG_DEBUG,
           "The \"user\" directive makes sense only "
           "if the master process runs "
           "with super-user privileges, ignored.");
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

s32 msf_set_rlimit(struct process *proc) {

    struct rlimit     rlmt;

    if (proc->rlimit_nofile != -1) {
        rlmt.rlim_cur = (rlim_t) proc->rlimit_nofile;
        rlmt.rlim_max = (rlim_t) proc->rlimit_nofile;

        //RLIMIT_NOFILE指定此进程可打开的最大文件描述词大一的值,超出此值,将会产生EMFILE错误。
        if (setrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
        }
    }

    if (proc->rlimit_core != -1) {
        rlmt.rlim_cur = (rlim_t) proc->rlimit_core;
        rlmt.rlim_max = (rlim_t) proc->rlimit_core;
       //修改工作进程的core文件尺寸的最大值限制(RLIMIT_CORE),用于在不重启主进程的情况下增大该限制
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

    process_pin_to_cpu(proc->cpu_affinity);

    /* allow coredump after setuid() in Linux 2.4.x */
    msf_enable_coredump();

    return 0;
}

s32 msf_system_init() {

    u32 n;

    struct rlimit   rlmt;
    struct utsname  u;

    if (uname(&u) == -1) {
        MSF_OS_LOG(DBG_ERROR, "Uname failed, errno(%d).", errno);
        return -1;
    }

    memcpy(g_os->sysname, (u8 *) u.sysname,
          min(sizeof(g_os->sysname), strlen(u.sysname)));
    memcpy(g_os->nodename, (u8 *) u.nodename,
          min(sizeof(g_os->nodename), strlen(u.nodename)));
    memcpy(g_os->release, (u8 *) u.release,
          min(sizeof(g_os->release), strlen(u.release)));
    memcpy(g_os->version, (u8 *) u.version,
          min(sizeof(g_os->version), strlen(u.version)));
    memcpy(g_os->machine, (u8 *) u.machine,
          min(sizeof(g_os->machine), strlen(u.machine)));
    memcpy(g_os->domainname, (u8 *) u.domainname,
          min(sizeof(g_os->domainname), strlen(u.domainname)));

#ifdef WIN32
    SYSTEM_INFO info; 
    GetSystemInfo(&info); 
    g_os->cpuonline = info.dwNumberOfProcessors;
#endif

    /* GNU fuction 
    * getpagesize(); numa_pagesize()
    * get_nprocs_conf();
    * get_nprocs();
    */
    g_os->pagesize = sysconf(_SC_PAGESIZE);
    g_os->pagenum_all = sysconf(_SC_PHYS_PAGES);
    g_os->pagenum_ava = sysconf(_SC_AVPHYS_PAGES);
    g_os->memsize = g_os->pagesize * g_os->pagenum_all / MB;

    /* Gather number of processors and CPU ticks linux2.6 Later
     refer:http://blog.csdn.net/u012317833/article/details/39476831 */
    g_os->cpuconf = sysconf(_SC_NPROCESSORS_CONF);
    g_os->cpuonline = sysconf(_SC_NPROCESSORS_ONLN);
    g_os->maxfileopen = sysconf(_SC_OPEN_MAX);
    g_os->tickspersec = sysconf(_SC_CLK_TCK);
    g_os->maxhostname = sysconf(_SC_HOST_NAME_MAX);
    g_os->maxloginname = sysconf(_SC_LOGIN_NAME_MAX);

    if (getrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
        MSF_OS_LOG(DBG_ERROR, "Getrlimit failed, errno(%d).", errno);
        return -1;
    }

    g_os->maxsocket = (s32) rlmt.rlim_cur;

    return 0;
}

/* Numa Info*/
void msf_vnode_init(void) {

    s32 rc;

    g_os->en_numa = numa_available();
    if (g_os->en_numa < 0) {
        MSF_OS_LOG(DBG_ERROR, "Your system does not support NUMA API.");
        return;
    }

    g_os->numacnt = numa_max_node();

    MSF_OS_LOG(DBG_DEBUG, "System numa en(%d).", g_os->en_numa);
    MSF_OS_LOG(DBG_DEBUG, "System numa num(%d).", g_os->numacnt);

    s32 nd;
    s8 *man = numa_alloc(1000);
    *man = 1;
    if (get_mempolicy(&nd, NULL, 0, man, MPOL_F_NODE|MPOL_F_ADDR) < 0)
        perror("get_mempolicy");
    else
        MSF_OS_LOG(DBG_DEBUG, "my node %d.", nd);

    msf_numa_free(man, 1000);

}

void msf_os_debug(void) {
    
    MSF_OS_LOG(DBG_DEBUG, "OS type:%s.",            g_os->sysname);
    MSF_OS_LOG(DBG_DEBUG, "OS nodename:%s.",        g_os->nodename);
    MSF_OS_LOG(DBG_DEBUG, "OS release:%s.",         g_os->release);
    MSF_OS_LOG(DBG_DEBUG, "OS version:%s.",         g_os->version);
    MSF_OS_LOG(DBG_DEBUG, "OS machine:%s.",         g_os->machine);
    MSF_OS_LOG(DBG_DEBUG, "OS domainname:%s.",      g_os->domainname);
    MSF_OS_LOG(DBG_DEBUG, "Processors conf:%u.",    g_os->cpuconf);
    MSF_OS_LOG(DBG_DEBUG, "Processors avai:%u.",    g_os->cpuonline);
    MSF_OS_LOG(DBG_DEBUG, "Cacheline size: %u.",    g_os->cacheline_size);
    MSF_OS_LOG(DBG_DEBUG, "Pagesize: %u.",          g_os->pagesize);
    MSF_OS_LOG(DBG_DEBUG, "Pages all num: %u",      g_os->pagenum_all);
    MSF_OS_LOG(DBG_DEBUG, "Pages available: %u.",   g_os->pagenum_ava);
    MSF_OS_LOG(DBG_DEBUG, "Memory size: %llu MB.",  g_os->memsize);
    MSF_OS_LOG(DBG_DEBUG, "Files max opened: %u.",  g_os->maxfileopen);
    MSF_OS_LOG(DBG_DEBUG, "Socket max opened: %u.", g_os->maxsocket);
    MSF_OS_LOG(DBG_DEBUG, "Ticks per second: %u.",  g_os->tickspersec);
    MSF_OS_LOG(DBG_DEBUG, "Maxlen host name: %u.",  g_os->maxhostname);
    MSF_OS_LOG(DBG_DEBUG, "Maxlen login name: %u.", g_os->maxloginname);

    MSF_OS_LOG(DBG_DEBUG, "Memory name1: %s.",      g_os->meminfo.name1);
    MSF_OS_LOG(DBG_DEBUG, "Memory total: %u.",      g_os->meminfo.total);
    MSF_OS_LOG(DBG_DEBUG, "Memory name2: %s.",      g_os->meminfo.name2);
    MSF_OS_LOG(DBG_DEBUG, "Memory free: %lu.",      g_os->meminfo.free);
    MSF_OS_LOG(DBG_DEBUG, "Memory used rate: %u.",  g_os->meminfo.used_rate);

    MSF_OS_LOG(DBG_DEBUG, "Hdd total: %lu.",        g_os->hdd.total);
    MSF_OS_LOG(DBG_DEBUG, "Hdd used_rate: %u.",     g_os->hdd.used_rate);
}

s32 msf_os_init(void) {

    msf_system_init();

    msf_vnode_init();

    msf_get_meminfo();

    msf_get_hdinfo();

    msf_get_cpuinfo();

    msf_os_debug();

    msf_cpu_perf_init();
    msf_cpu_perf_collect();

    return 0;
}

struct msf_os *msf_get_os(void) {
    return g_os;
}

