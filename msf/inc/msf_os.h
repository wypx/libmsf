
#include <msf_file.h>
#include <msf_process.h>

#include <sched.h>
#include <grp.h>
#include <pwd.h>
#include <sys/sysinfo.h>
#include <sys/resource.h>
#include <sys/utsname.h>

extern u32 msf_cacheline_size;

struct msf_meminfo {
    s8      name1[20];
    double  total;
    s8      name2[20];
    double  free;
    double  used_rate;
} MSF_PACKED_MEMORY;

/** Memory information structure */
struct msf_mem {
    u32 total_size; /** total size of memory pool */
    u32 free;        /** free memory */
    u32 used;       /** allocated size */
    u32 real_used;  /** used size plus overhead from malloc */
    u32 max_used;   /** maximum used size since server start? */
    u32 min_frag;   /** minimum number of fragmentations? */
    u32 total_frags; /** number of total memory fragments */
} MSF_PACKED_MEMORY;

struct msf_hdd {
    double total;
    double used_rate;
} MSF_PACKED_MEMORY;

struct msf_os {
    u8      sysname[64];
    u8      nodename[64];
    u8      release[64];
    u8      version[64];
    u8      machine[64];
    u8      domainname[_UTSNAME_DOMAIN_LENGTH];
    
    u32     cacheline_size;
    u32     pagesize;
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

    struct msf_mem  mem;
    struct msf_hdd  hdd;
} MSF_PACKED_MEMORY;


s32 msf_get_meminfo(struct msf_meminfo *mem);
s32 msf_get_hdinfo(struct msf_hdd *hd);

void msf_cpuinfo(void);
s32 msf_set_user(struct process *proc);
s32 msf_os_init(void);

