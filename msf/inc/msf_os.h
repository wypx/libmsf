
#include <msf_process.h>
#include <msf_file.h>


#include <sched.h>  
#include <pthread.h> 

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
}__attribute__((__packed__));

/** Memory information structure */
struct mem_info{
    unsigned long total_size; /** total size of memory pool */
    unsigned long free; /** free memory */
    unsigned long used; /** allocated size */
    unsigned long real_used; /** used size plus overhead from malloc */
    unsigned long max_used; /** maximum used size since server start? */
    unsigned long min_frag; /** minimum number of fragmentations? */
    unsigned long total_frags; /** number of total memory fragments */
};

struct msf_hdinfo {
    double total;
    double used_rate;
}__attribute__((__packed__));

s32 msf_get_meminfo(struct msf_meminfo *mem);
s32 msf_get_hdinfo(struct msf_hdinfo *hd);

void msf_cpuinfo(void);
s32 msf_set_user(struct process *proc);

