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
    log_write(level, MSF_MOD_UTILS, MSF_FUNC_FILE_LINE, __VA_ARGS__)

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
    
    if (*fd_output >= 0 || *fd_stdout >= 0) {
        return -1;
    }
    *fd_output  = open(device_name, O_WRONLY | O_NONBLOCK);	
    if (*fd_output  < 0) {
        return -1;
    }

    *fd_stdout = dup(STDOUT_FILENO);
    if (*fd_stdout < 0) {
        MSF_UTILS_LOG(DBG_ERROR, "err in dup STDOUT.");
        close(*fd_output);
        *fd_output = -1;
        return -1;
    }

    if (dup2(*fd_output, STDOUT_FILENO) < 0){
        MSF_UTILS_LOG(DBG_ERROR, "err in dup STDOUT.");
        close(*fd_output);
        *fd_output = -1;
        *fd_stdout = -1;
        return -1;
    }

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

s32 rdirection_output_cancle(s32* fd_output, s32* fd_stdout)
{
    char szDevDsc = 0;

    if (!fd_output || !fd_stdout){
        return -1;
    }

    if (*fd_output < 0 || *fd_stdout < 0){
        return -1;
    }
    
    if (dup2(*fd_output, STDOUT_FILENO) < 0){
        MSF_UTILS_LOG(DBG_ERROR, "<rdirection_output_cancle> err in cancle redirection STDOUT.\n");
        return -1;
    }
    if (dup2(*fd_stdout, STDERR_FILENO) < 0){
        MSF_UTILS_LOG(DBG_ERROR, "<rdirection_output_cancle> err in cancle redirection STDERR.\n");
        return -1;
    }

    sclose(*fd_output);
    sclose(*fd_stdout);

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
     * 防止守护进程没有正常运作之前控制终端受到干扰退出或挂起 */
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
     * 目标: 后台运行。
     * 做法:   脱离控制终端->调用fork->终止父进程->子进程被init收养.
     *
     * 目标: 脱离控制终端, 登陆会话和进程组
     * 做法: 使用setsid创建新会话,成为新会话的首进程,则与原来的
     *       登陆会话和进程组自动脱离,从而脱离控制终端.
     *       一步的fork保证了子进程不可能是一个会话的首进程,
     *       这是调用setsid的必要条件.
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



