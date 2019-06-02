#include <msf_utils.h>

#define MSF_INVALID_PID     -1
#define MSF_MAX_PROCESSES   64

enum msf_process_type {
    MSF_PROCESS_MASTER = 0,
    MSF_PROCESS_ROUTER,
    MSF_PROCESS_WORKER,
    MSF_PROCESS_MAXIDX,
};

struct exec_conf {
    s8         *path;
    s8         *name;
    s8 *const  *argv;
    s8 *const  *envp;
} MSF_PACKED_MEMORY;

enum process_state {
    proc_state_init,
    proc_state_start,
    proc_state_stop,
} MSF_PACKED_MEMORY;


struct process_desc {
    pid_t   proc_pid;
    time_t  proc_age;
    s32     proc_fd;
    s32     restart_times;

    u32     proc_idx;
    s8      proc_name[64];
    s8      proc_path[64]; /* Check access first*/
    s8      proc_conf[64]; /* Check access first*/
} MSF_PACKED_MEMORY;

struct process {
    enum process_state proc_state;

    pid_t  pid;
    u32 role; /* master, slave */
    u32 worker_processes;
    s8  *working_directory;
    s32 channel[2];

    s8  *username;
    uid_t user;
    gid_t group;

    s32 priority; /* nice of process or thread */
    s32 cpu_affinity;
    s32 cpu_affinity_id;

    u32 coredump;
    s8  *core_file;
    s8  *lock_file;
    //修改工作进程的打开文件数的最大值限制(RLIMIT_NOFILE),用于在不重启主进程的情况下增大该限制。
    s32 rlimit_nofile;
    //修改工作进程的core文件尺寸的最大值限制(RLIMIT_CORE),用于在不重启主进程的情况下增大该限制。
    s32 rlimit_core;

    s8  proc_name[32];
    s8  proc_author[32];
    s8  proc_desc[32];
    s8  proc_version[32];

    s32 logfd;

    s8  *confile;
    s32 proc_signo;
    s32 proc_daemon;
    u32 proc_upgrade;
    u32 terminate;
    u32 quit; /* gracefully shutting down */
    u32 fault_times;
    u32 alarm_times;
    u32 alarm_state;
    u32 start_times;

    u32 proc_svc_num;
    struct svcinst *proc_svcs;
} MSF_PACKED_MEMORY;

s32 process_try_lock(const s8 *proc_name, u32 mode);
s32 process_spwan(struct process_desc *proc_desc);

