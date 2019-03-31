/**************************************************************************
*
* Copyright (c) 2017-2018, luotang.me <wypx520@gmail.com>, China.
* All rights reserved.
*
* Distributed under the terms of the GNU General Public License v2.
*
* This software is provided 'as is' with no explicit or implied warranties
* in respect of its properties, including, but not limited to, correctness
* and/or fitness for purpose.
*
**************************************************************************/
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <msf_log.h>

#define MSF_MOD_UTILS "UTILS"
#define MSF_UTILS_LOG(level, ...) \
    log_write(level, MSF_MOD_UTILS, __func__, __FILE__, __LINE__, __VA_ARGS__)

void msf_touppercase(s8 *s) {
    while (*s) {
        *s = (s8)toupper(*s);
        ++s;
    }
}

void msf_nsleep(s32 ns) {
    struct timespec req;
    req.tv_sec = 0;
    req.tv_nsec = ns;
    nanosleep(&req, 0);
}

void msf_susleep(s32 s, s32 us) {
    struct timeval tv;
    tv.tv_sec = s;
    tv.tv_usec = us;
    select(0, NULL, NULL, NULL, &tv);
}

#if defined (__linux__)
#include <sys/prctl.h>
#elif defined (WIN32)
  #ifndef localtime_r
    struct tm *localtime_r(time_t *_clock, struct tm *_result)
    {
        struct tm *p = localtime(_clock);
        if (p)
            *(_result) = *p;
        return p;
    }
  #endif
#endif

s32 signal_handler(s32 sig, sighandler_t handler) {
    MSF_UTILS_LOG(DBG_INFO, "signal handled: %s.", strsignal(sig));
    struct sigaction action;
    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);
    /* 安装信号的时候, 设置 SA_RESTART属性, 
     * 那么当信号处理函数返回后, 被该信号中断的系统调用将自动恢复*/
    action.sa_flags =  SA_NODEFER | SA_RESTART;
    sigaction(sig, &action, NULL);

    return 0;
}

s8 *config_read_file(const s8 *filename) {
    FILE *file = NULL;
    s64 length = 0;
    s8 *content = NULL;
    size_t read_chars = 0;

    /* open in read binary mode */
    file = fopen(filename, "rb");
    if (!file) {
        goto cleanup;
    }

    /* get the length */
    if (fseek(file, 0, SEEK_END) != 0) {
        goto cleanup;
    }

    length = ftell(file);
    if (length < 0) {
        goto cleanup;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        goto cleanup;
    }

    /* allocate content buffer */
    content = (s8*)malloc((size_t)length + 1);
    if (!content) {
        goto cleanup;
    }

    memset(content, 0, (size_t)length + 1);

    /* read the file into memory */
    read_chars = fread(content, sizeof(char), (size_t)length, file);
    if ((long)read_chars != length) {
        sfree(content);
        goto cleanup;
    }

    content[read_chars] = '\0';

cleanup:
    sfclose(file);
    return content;
}

s32 file_get_info(const s8 *path, struct file_info *f_info, u32 mode) {
    struct stat f, target;

    f_info->exists = false;

    /* Stat right resource */
    if (lstat(path, &f) == -1) {
        if (errno == EACCES) {
            f_info->exists = true;
        }
        return -1;
    }

    f_info->exists = true;
    f_info->is_file = true;
    f_info->is_link = false;
    f_info->is_directory = false;
    f_info->exec_access = false;
    f_info->read_access = false;

    if (S_ISLNK(f.st_mode)) {
        f_info->is_link = true;
        f_info->is_file = false;
        if (stat(path, &target) == -1) {
            return -1;
        }
    }
    else {
        target = f;
    }

    f_info->size = target.st_size;
    f_info->last_modification = target.st_mtime;

    if (S_ISDIR(target.st_mode)) {
        f_info->is_directory = true;
        f_info->is_file = false;
    }

#ifndef _WIN32
    gid_t EGID = getegid();
    gid_t EUID = geteuid();

    /* Check read access */
    if (mode & MSF_FILE_READ) {
        if (((target.st_mode & S_IRUSR) && target.st_uid == EUID) ||
            ((target.st_mode & S_IRGRP) && target.st_gid == EGID) ||
            (target.st_mode & S_IROTH)) {
            f_info->read_access = true;
        }
    }

    /* Checking execution */
    if (mode & MSF_FILE_EXEC) {
        if ((target.st_mode & S_IXUSR && target.st_uid == EUID) ||
            (target.st_mode & S_IXGRP && target.st_gid == EGID) ||
            (target.st_mode & S_IXOTH)) {
            f_info->exec_access = true;
        }
    }
#endif

    /* Suggest open(2) flags */
    f_info->flags_read_only = O_RDONLY | O_NONBLOCK;

#if defined(__linux__)
    /*
     * If the user is the owner of the file or the user is root, it
     * can set the O_NOATIME flag for open(2) operations to avoid
     * inode updates about last accessed time
     */
    if (target.st_uid == EUID || EUID == 0) {
        f_info->flags_read_only |=  O_NOATIME;
    }
#endif

    return 0;
}


void* plugin_load_dynamic(const s8 *path) {
    void *handle = NULL;

    handle = MSF_DLOPEN_L(path);
    if (!handle) {
        MSF_UTILS_LOG(DBG_ERROR, "DLOPEN_L() %s.", MSF_DLERROR());
    }

    return handle;
}

void *plugin_load_symbol(void *handler, const s8 *symbol)
{
    void *s = NULL;

    MSF_DLERROR();/* Clear any existing error */

    /* Writing: cosine = (double (*)(double)) dlsym(handle, "cos");
       would seem more natural, but the C99 standard leaves
       casting from "void *" to a function pointer undefined.
       The assignment used below is the POSIX.1-2003 (Technical
       Corrigendum 1) workaround; see the Rationale for the
       POSIX specification of dlsym(). */
       
    s = MSF_DLSYM(handler, symbol);

    if (!s) {
        MSF_UTILS_LOG(DBG_ERROR, "DLSYM() %s.", MSF_DLERROR());
    }
    return s;
}

s32 check_file_exist(s8 *file) {

    if (unlikely(file)) return -1;

    return (access(file, R_OK | W_OK) >= 0) ? 0 : -1;
}

static s8 szDevPrint[128];

s8 *sys_output_dev(const s8 *pDevDsc) {
    /*sys output dev set*/
    if (NULL != pDevDsc) {
        if ((strlen(pDevDsc) > 0) && (strlen(pDevDsc) < sizeof(szDevPrint))){
            strcpy(szDevPrint, pDevDsc);
            MSF_UTILS_LOG(DBG_ERROR, "sys output dev set (%s).", szDevPrint);
        }else{
            szDevPrint[0] = 0;
        }
    }

    /*sys output dev get*/
    if ((strlen(szDevPrint) > 0) && (strlen(szDevPrint) < sizeof(szDevPrint))){
        return szDevPrint;
    }

    return NULL;
}

s32 redirection_output(const s8* device_name, s32* fd_output, s32* fd_stdout) {
    if (!device_name || !fd_output || !fd_stdout) {
        return -1;
    }

    /* 如果已经打开描述符 */
    if (*fd_output >= 0 || *fd_stdout >= 0) {
        return -1;
    }

    /* 打开输出设备 */
    *fd_output  = open(device_name, O_WRONLY | O_NONBLOCK);	
    if (*fd_output  < 0) {
        return -1;
    }

    /* 复制标准输出描述符 */		
    *fd_stdout = dup(STDOUT_FILENO);
    if (*fd_stdout < 0) {
        MSF_UTILS_LOG(DBG_ERROR, "err in dup STDOUT.");
        close(*fd_output);
        *fd_output = -1;
        return -1;
    }

    /*复制打开设备的描述符，并指定其值为 标准输出描述符 */
    if (dup2(*fd_output, STDOUT_FILENO) < 0){
        MSF_UTILS_LOG(DBG_ERROR, "err in dup STDOUT.");
        close(*fd_output);
        *fd_output = -1;
        *fd_stdout = -1;
        return -1;
    }

    /*复制打开设备的描述符，并指定其值为 标准错误描述符 */
    if (dup2(*fd_output, STDERR_FILENO) < 0){
        MSF_UTILS_LOG(DBG_ERROR, "err in dup STDERR.");
        close(*fd_output);
        *fd_output = -1;
        *fd_stdout = -1;
        return -1;
    }

    if (NULL == sys_output_dev(device_name)){
        return -1;
    }

    return 0;
}

int rdirection_output_cancle(int* fd_output, int* fd_stdout)
{
    char szDevDsc = 0;

    if (!fd_output || !fd_stdout){
        return -1;
    }

    /* 参数检查 */
    if (*fd_output < 0 || *fd_stdout < 0){
        return -1;
    }

    /* 关闭重定向标准输出的设备，用保存的标准输出描述符的值来恢复标准输出 */
    if (dup2(*fd_stdout, STDOUT_FILENO) < 0){
        MSF_UTILS_LOG(DBG_ERROR, "<rdirection_output_cancle> err in cancle redirection STDOUT.\n");
        return -1;
    }

    /* 关闭重定向标准输出的设备，用保存的标准输出描述符的值来恢复标准错误 */
    if (dup2(*fd_stdout, STDERR_FILENO) < 0){
        MSF_UTILS_LOG(DBG_ERROR, "<rdirection_output_cancle> err in cancle redirection STDERR.\n");
        return -1;
    }

    /* 释放资源 */
    close(*fd_output);
    close(*fd_stdout);

    *fd_output = -1;	/* 置为无效的fd */
    *fd_stdout = -1;	/* 置为无效的fd */

    if (NULL != sys_output_dev(&szDevDsc)){
        return -1;
    }

    return 0;
}

s32 daemonize(s32 nochdir, s32 noclose)
{
    s32 fd = -1;

#ifdef SIGTTOU
/* 下面用于屏蔽一些有关控制终端操作的信号
防止守护进程没有正常运作之前控制终端受到干扰退出或挂起 */
    signal(SIGTTOU, SIG_IGN); //忽略后台进程写控制终端信号
#endif

#ifdef SIGTTIN
    signal(SIGTTIN, SIG_IGN); //忽略后台进程读控制终端信号
#endif
#ifdef SIGTSTP
    signal(SIGTSTP, SIG_IGN); //忽略终端挂起
#endif

#ifndef USE_DAEMON

    /* 下面开始从普通进程转换为守护进程
     * 目标1：后台运行。
     * 做法：脱离控制终端->调用fork之后终止父进程
     * 子进程被init收养，此步达到后台运行的目标。
     */

    /*fork() != 0 目标2：脱离控制终端，登陆会话和进程组。
     * 做法：使用setsid创建新会话，成为新会话的首进程，则与原来的
     * 登陆会话和进程组自动脱离，从而脱离控制终端。
     * （上一步的fork保证了子进程不可能是一个会话的首进程，这是调用setsid的必要条件）
     */
    s32 pipefd[2];
    pid_t pid;

    if (pipe(pipefd) < 0) exit(-1);

    if (0 > (pid = fork())) exit(-1);

    if (pid > 0) {
        s8 buf;
        ssize_t bytes;

        close(pipefd[1]);
        /* parent waits for grandchild to be ready */
    do {
        bytes = read(pipefd[0], &buf, sizeof(buf));
    } while (bytes < 0 && EINTR == errno);
    close(pipefd[0]);

    if (bytes <= 0) {
        /* closed fd (without writing) == failure in grandchild */
        MSF_UTILS_LOG(DBG_ERROR, "Failed to daemonize.");
        exit(-1);
    }

        exit(0);
    }
    close(pipefd[0]);

    /* set files mask */
    umask(0);

    if (setsid() == -1) exit(0);

    signal(SIGHUP, SIG_IGN);

    if (0 != fork()) exit(0);

    /* 上面已经完成了大部分工作，但是有的系统上，当会话首进程打开
     * 一个尚未与任何会话相关联的终端设备时，该设备自动作为控制
     * 终端分配给该会话。
     * 为避免该情况，我们再次fork进程，于是新进程不再是会话首进程。
     * 会话首进程退出时可能会给所有会话内的进程发送SIGHUP，而该
     * 信号默认是结束进程，故需要忽略该信号来防止孙子进程意外结束。
     */

    /* 最后目标：改变工作目录到根目录。
     * 原因：进程活动时，其工作目录所在的文件系统不能卸下。
     */

    if (nochdir == 0) {
        if(chdir("/") != 0) {
            perror("chdir");
            exit(EXIT_FAILURE);
        }
    }

    //fclose(stderr);
    //fclose(stdout);
    if (noclose == 0 && (fd = open("/dev/null", O_RDWR, 0)) != -1) {
        if(dup2(fd, STDIN_FILENO) < 0) {
            MSF_UTILS_LOG(DBG_ERROR, "dup2 stdin");
            return (-1);
        }
        if(dup2(fd, STDOUT_FILENO) < 0) {
            MSF_UTILS_LOG(DBG_ERROR, "dup2 stdout");
            return (-1);
        }
        if(dup2(fd, STDERR_FILENO) < 0) {
            MSF_UTILS_LOG(DBG_ERROR, "dup2 stderr");
            return (-1);
        }

        if (fd > STDERR_FILENO) {
            if(close(fd) < 0) {
                MSF_UTILS_LOG(DBG_ERROR, "close fd failed");
                return (-1);
            }
        }
    }
#else
    if (daemon(0, 0) < 0) {
        MSF_UTILS_LOG(DBG_ERROR, "daemon()");
    }
#endif

    //fdevent_setfd_cloexec(pipefd[1]);
    //return pipefd[1];
    return (0);
}

s32 sem_wait_i(sem_t *psem, s32 mswait) {
    s32 rv = 0;

    if (unlikely(!psem)) 
        return -1;

    if (MSF_NO_WAIT == mswait) {
    while ((rv = sem_trywait(psem)) != 0  && errno == EINTR) {
            usleep(1000);
    }
    } else if (MSF_WAIT_FOREVER == mswait) {
        while ((rv = sem_wait(psem)) != 0  && errno == EINTR) {
            usleep(1000);
        }
    } else {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts );
        ts.tv_sec += (mswait / 1000 );
        ts.tv_nsec += ( mswait % 1000 ) * 1000;
        while ((rv = sem_timedwait(psem, &ts)) != 0 && errno == EINTR) {
                usleep(1000);
        }
    } 
    return rv;
} 

void save_pid(const s8 *pid_file) {
    FILE *fp = NULL;
    if (access(pid_file, F_OK) == 0) {
        if ((fp = fopen(pid_file, "r")) != NULL) {
            s8 buffer[1024];
            if (fgets(buffer, sizeof(buffer), fp) != NULL) {
                unsigned int pid;
                if (safe_strtoul(buffer, &pid) && kill((pid_t)pid, 0) == 0) {
                    MSF_UTILS_LOG(DBG_ERROR, "WARNING: The pid file contained the following (running) pid: %u\n", pid);
                }
            }
            fclose(fp);
        }
    }

    /* Create the pid file first with a temporary name, then
     * atomically move the file to the real name to avoid a race with
     * another process opening the file to read the pid, but finding
     * it empty.
     */
    s8 tmp_pid_file[1024];
    snprintf(tmp_pid_file, sizeof(tmp_pid_file), "%s.tmp", pid_file);

    if ((fp = fopen(tmp_pid_file, "w")) == NULL) {
        MSF_UTILS_LOG(DBG_ERROR, "Could not open the pid file %s for writing", tmp_pid_file);
        return;
    }

    fprintf(fp,"%ld\n", (long)getpid());
    if (fclose(fp) == -1) {
        MSF_UTILS_LOG(DBG_ERROR, "Could not close the pid file %s", tmp_pid_file);
    }

    if (rename(tmp_pid_file, pid_file) != 0) {
        MSF_UTILS_LOG(DBG_ERROR, "Could not rename the pid file from %s to %s",
                tmp_pid_file, pid_file);
    }
}

void remove_pidfile(const s8 *pid_file) {
    if (unlikely(!pid_file))
      return;

    if (access(pid_file, F_OK) == 0) {
        if (unlink(pid_file) != 0) {
            MSF_UTILS_LOG(DBG_ERROR, "Could not remove the pid file %s", pid_file);
        }
    }
}

/* Avoid warnings on solaris, where isspace() is an index into an array, and gcc uses signed chars */
#define xisspace(c) isspace((u8)c)

u8 safe_strtoull(const s8 *str, u64 *out) {
    if (!str) return -1;

    errno = 0;
    *out = 0;
    s8*endptr;
    u64 ull = strtoull(str, &endptr, 10);
    if ((errno == ERANGE) || (str == endptr)) {
        return false;
    }

    if (xisspace(*endptr) || (*endptr == '\0' && endptr != str)) {
        if ((long long) ull < 0) {
            /* only check for negative signs in the uncommon case when
             * the unsigned number is so big that it's negative as a
             * signed number. */
            if (strchr(str, '-') != NULL) {
                return false;
            }
        }
        *out = ull;
        return true;
    }
    return false;
}

u8 safe_strtoll(const s8 *str, s64 *out) {
    if (!out) return -1;

    errno = 0;
    *out = 0;
    s8 *endptr;
    s64 ll = strtoll(str, &endptr, 10);
    if ((errno == ERANGE) || (str == endptr)) {
        return false;
    }

    if (xisspace(*endptr) || (*endptr == '\0' && endptr != str)) {
        *out = ll;
        return true;
    }
    return false;
}

u8 safe_strtoul(const s8 *str, u32 *out) {
    s8 *endptr = NULL;
    unsigned long l = 0;

    if (!str || !out) return -1;

    *out = 0;
    errno = 0;

    l = strtoul(str, &endptr, 10);
    if ((errno == ERANGE) || (str == endptr)) {
        return false;
    }

    if (xisspace(*endptr) || (*endptr == '\0' && endptr != str)) {
        if ((long) l < 0) {
            /* only check for negative signs in the uncommon case when
             * the unsigned number is so big that it's negative as a
             * signed number. */
            if (strchr(str, '-') != NULL) {
                return false;
            }
        }
        *out = l;
        return true;
    }

    return false;
}

u8 safe_strtol(const s8 *str, s32 *out) {
    if (!out) return -1;

    errno = 0;
    *out = 0;
    s8 *endptr;
    long l = strtol(str, &endptr, 10);
    if ((errno == ERANGE) || (str == endptr)) {
        return false;
    }

    if (xisspace(*endptr) || (*endptr == '\0' && endptr != str)) {
        *out = l;
        return true;
    }
    return false;
}

/* Display a buffer into a HEXA formated output */
void dump_buffer(s8 *buff, size_t count, FILE* fp)
{
    size_t i, j, c;
    u8 printnext = true;

    if (count % 16)
        c = count + (16 - count % 16);
    else
        c = count;

    for (i = 0; i < c; i++) {
        if (printnext) {
            printnext = false;
            fprintf(fp, "%.4zu ", i & 0xffff);
        }
        if (i < count)
            fprintf(fp, "%3.2x", buff[i] & 0xff);
        else
            fprintf(fp, "   ");
        if (!((i + 1) % 8)) {
            if ((i + 1) % 16)
                fprintf(fp, " -");
            else {
                fprintf(fp, "   ");
                for (j = i - 15; j <= i; j++) {
                    if (j < count) {
                        if ((buff[j] & 0xff) >= 0x20
                            && (buff[j] & 0xff) <= 0x7e)
                            fprintf(fp, "%c",
                                   buff[j] & 0xff);
                        else
                            fprintf(fp, ".");
                    } else
                        fprintf(fp, " ");
                }
                fprintf(fp, "\n");
                printnext = true;
            }
        }
    }
}

#include <execinfo.h>
void write_stacktrace(const s8 *file_name) {
    int fd = open(file_name, O_WRONLY | O_APPEND | O_CREAT, 0644);
    void *buffer[100];
    int nptrs;

    nptrs = backtrace(buffer, 100);
    backtrace_symbols_fd(buffer, nptrs, fd);
    if (write(fd, "\n", 1) != 1) {
        /* We don't care, but this stops a warning on Ubuntu */
    }
    close(fd);
}

void close_std_fd(void) {
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

s32 pthread_spawn(pthread_t *tid, void* (*func)(void *), void *arg) {
    s32 rc;
    pthread_attr_t thread_attr;

#ifndef WIN32
    sigset_t signal_mask;
    sigemptyset (&signal_mask);
    sigaddset (&signal_mask, SIGPIPE);
    rc = pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);
    if (rc != 0) {
        MSF_UTILS_LOG(DBG_ERROR, "block sigpipe error\n");
    } 
#endif

    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
    if (pthread_create(tid, &thread_attr, (void *)func, arg) < 0) {
        MSF_UTILS_LOG(DBG_ERROR, "pthread_create");
        return -1;
    }

    return 0;
}


//https://blog.csdn.net/hzhsan/article/details/25124901
//#define refcount_incr(it) ++(it->refcount)
//#define refcount_decr(it) --(it->refcount)

#ifdef __sun
#include <atomic.h>
#endif


#if !defined(HAVE_GCC_ATOMICS) && !defined(__sun)
pthread_mutex_t atomics_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

u32 refcount_incr(u32 *refcount) {
#ifdef HAVE_GCC_ATOMICS
    return __sync_add_and_fetch(refcount, 1);
#elif defined(__sun)
    return atomic_inc_ushort_nv(refcount);
#else
    u32 rc;
    pthread_mutex_trylock(&atomics_mutex);
    (*refcount)++;
    rc = *refcount;
    pthread_mutex_trylock(&atomics_mutex);
    return rc;
#endif
}

u32 refcount_decr(u32 *refcount) {
#ifdef HAVE_GCC_ATOMICS
    return __sync_sub_and_fetch(refcount, 1);
#elif defined(__sun)
    return atomic_inc_ushort_nv(refcount);
#else
    u32 rc;
    pthread_mutex_trylock(&atomics_mutex);
    (*refcount)--;
    rc = *refcount;
    pthread_mutex_trylock(&atomics_mutex);
    return rc;
#endif
}



