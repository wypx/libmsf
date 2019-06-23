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
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/sysinfo.h>
#include <msf_process.h>
#include <msf_network.h>

s32 msf_lockfile(s32 fd) {
    struct flock fl;

    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;

    return fcntl(fd, F_SETLK, &fl);
}

/* flock锁的释放非常具有特色:
 * 即可调用LOCK_UN参数来释放文件锁
 * 也可以通过关闭fd的方式来释放文件锁
 * 意味着flock会随着进程的关闭而被自动释放掉 */
s32 process_try_lock(const s8 *proc_name, u32 mode) {

    s32 rc;
    s32 fd = -1;
    s8 proc_file[64] = { 0 };
    snprintf(proc_file, sizeof(proc_file)-1, "/var/run/%s", proc_name);

    /*Linux/Unix文件系统中，有一种排它锁：WRLCK，
    只允许一次加锁成功，而且当进程无论主动退出还是被动退出,
    都会由操作系统释放。
    这种锁作用于pid文件上，特别适合于防止启动进程的多于一个副本
    防止进程启动多个副本。只有获得pid文件(固定路径固定文件名)
    写入权限(F_WRLCK)的进程才能正常启动并把自身的PID写入该文件中。
    其它同一个程序的多余进程则自动退出
    flock，建议性锁，不具备强制性.
    一个进程使用flock将文件锁住，另一个进程可以直接操作正在被锁的文件，
    修改文件中的数据，原因在于flock只是用于检测文件是否被加锁，
    针对文件已经被加锁，另一个进程写入数据的情况，
    内核不会阻止这个进程的写入操作，也就是建议性锁的内核处理策略
    flock主要三种操作类型：
    LOCK_SH，共享锁，多个进程可以使用同一把锁，常被用作读共享锁；
    LOCK_EX，排他锁，同时只允许一个进程使用，常被用作写锁；
    LOCK_UN，释放锁
    进程使用flock尝试锁文件时，如果文件已经被其他进程锁住，
    进程会被阻塞直到锁被释放掉，或者在调用flock的时候，
    采用LOCK_NB参数，在尝试锁住该文件的时候，发现已经被其他服务锁住，
    会返回错误，errno错误码为EWOULDBLOCK。
    即提供两种工作模式：阻塞与非阻塞类型
    flock锁的释放非常具有特色，即可调用LOCK_UN参数来释放文件锁，
    也可以通过关闭fd的方式来释放文件锁（flock的第一个参数是fd）,
    意味着flock会随着进程的关闭而被自动释放掉
    flock其中的一个使用场景为：检测进程是否已经存在
    在多线程开发中，互斥锁可以用于对临界资源的保护，防止数据的不一致，
    这是最为普遍的使用方法。那在多进程中如何处理文件之间的同步呢
    flock()的限制
    flock()放置的锁有如下限制
    只能对整个文件进行加锁。这种粗粒度的加锁会限制协作进程间的并发。
    假如存在多个进程，其中各个进程都想同时访问同一个文件的不同部分。
    通过flock()只能放置劝告式锁。
    很多NFS实现不识别flock()放置的锁。
    注释：在默认情况下，文件锁是劝告式的，这表示一个进程可以简单地忽略
    另一个进程在文件上放置的锁。要使得劝告式加锁模型能够正常工作，
    所有访问文件的进程都必须要配合，即在执行文件IO之前先放置一把锁。
    */
    fd = open(proc_file, O_RDWR | O_CREAT | O_CLOEXEC, 
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
        printf("Open prcoess lock file(%s) failed, errno(%d)\n", proc_name, errno);
        return true;
    }

    msf_socket_closeonexec(fd);

    if (0 == flock(fd, LOCK_EX | LOCK_NB)) {
        printf("(%s) has not been locked, lock it.\n", proc_file);
        rc = true;
    } else {
        printf("(%s) has locked.\n", proc_file);
        if (EACCES == errno || EAGAIN == errno || EWOULDBLOCK == errno) {

        }
        rc = false;
    }

    return rc;
}
void process_wait_child_termination(pid_t v_pid) {
    pid_t pid;
    s32 status;

    /*wait不能使用在while中,因为wait是阻塞的,比如当某子进程一直不停止，
    它将一直阻塞,从而影响了父进程.于是使用waitpid，options设置为WNOHANG,
    可以在非阻塞情况下处理多个退出子进程*/
    while ((pid = waitpid(v_pid, &status, WNOHANG))) {
        if (pid == -1)  {
            if (errno == EINTR)
                continue;
            else
                break;
        }
    }

    if (WIFEXITED(status)) {
        printf("Child(%d) normal exit(%d)\n", v_pid, WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("Child abnormally exit, signal(%d)\n", WTERMSIG(status));
    }
}



void process_reap_children(void)
{

}
#include <sys/socket.h>
#include <sys/prctl.h>

//http://www.cnblogs.com/mickole/p/3188321.html
s32 process_spwan_grandson(s32 wfd, struct process_desc *proc_desc) {

    pid_t grandson;

    grandson = fork();
    if (grandson > 0) {
        printf("Now child process exit, pid(%d).\n", getpid());
        /*子进程退出,孙子进程让init进程托管,避免僵尸进程*/
        exit(0);
    } else if (0 == grandson) {

        printf("Now grandson process(%s) pid(%d) enter.\n", 
        proc_desc->proc_name, getpid());

        pid_t pid = getpid();
        while (send(wfd, (s8*)&pid, sizeof(pid), MSG_NOSIGNAL) != sizeof(pid_t));

        s8 *argv[] = { proc_desc->proc_path, "-c", proc_desc->proc_conf, NULL };// 数组最后一位需要为0
        execvp(proc_desc->proc_path, argv);

        //execvp
        //exec系统调用会从当前进程中把当前程序的机器指令清除，
        //然后在空的进程中载入调用时指定的程序代码
        execv(proc_desc->proc_path, argv);
        printf("Now grandson process pid(%d) exit.\n", getpid());
        exit(-1);
    }

    return getpid();
}

s32 process_spwan(struct process_desc *proc_desc) {

        s32 rc;
        s32 fd[2];
        pid_t child;

        pid_t exit_child;
        s32 status;

#if 0
        if (proc_desc->restart_times++ > 0) {
            rc = process_try_lock(proc_desc->proc_name, 0);
            if (false == rc) {
                if (proc_desc->proc_pid > 0) {
                    printf("Now kill old process, pid(%d).\n", proc_desc->proc_pid);
                    kill(proc_desc->proc_pid, SIGKILL);
                }
            }
        }
#endif

    /*
      Making the second socket nonblocking is a bit subtle, given that we
      ignore any EAGAIN returns when writing to it, and you don't usally
      do that for a nonblocking socket. But if the kernel gives us EAGAIN,
      then there's no need to add any more data to the buffer, since
      the main thread is already either about to wake up and drain it,
      or woken up and in the process of draining it.
    */

    //socketpair(AF_UNIX, SOCK_STREAM, 0, ngx_processes[s].channel)

    //rc = pipe(fd);
    rc = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
    if (rc < 0) {  
        perror ("pipe error\n");  
        return -1;  
    }

    //nolocking fd 0,1
#if 0
	 /* 
            设置异步模式： 这里可以看下《网络编程卷一》的ioctl函数和fcntl函数 or 网上查询 
          */  
        on = 1; // 标记位，ioctl用于清除（0）或设置（非0）操作  

        /* 
          设置channel[0]的信号驱动异步I/O标志 
          FIOASYNC：该状态标志决定是否收取针对socket的异步I/O信号（SIGIO） 
          其与O_ASYNC文件状态标志等效，可通过fcntl的F_SETFL命令设置or清除 
         */ 
        if (ioctl(ngx_processes[s].channel[0], FIOASYNC, &on) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "ioctl(FIOASYNC) failed while spawning \"%s\"", name);
            ngx_close_channel(ngx_processes[s].channel, cycle->log);
            return NGX_INVALID_PID;
        }

        /* F_SETOWN：用于指定接收SIGIO和SIGURG信号的socket属主（进程ID或进程组ID） 
          * 这里意思是指定Master进程接收SIGIO和SIGURG信号 
          * SIGIO信号必须是在socket设置为信号驱动异步I/O才能产生，即上一步操作 
          * SIGURG信号是在新的带外数据到达socket时产生的 
         */ 
        if (fcntl(ngx_processes[s].channel[0], F_SETOWN, ngx_pid) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "fcntl(F_SETOWN) failed while spawning \"%s\"", name);
            ngx_close_channel(ngx_processes[s].channel, cycle->lo;
            return NGX_INVALID_PID;
        }
            
        
        /* FD_CLOEXEC：用来设置文件的close-on-exec状态标准 
          *             在exec()调用后，close-on-exec标志为0的情况下，此文件不被关闭；非零则在exec()后被关闭 
          *             默认close-on-exec状态为0，需要通过FD_CLOEXEC设置 
          *     这里意思是当Master父进程执行了exec()调用后，关闭socket        
          */
        if (fcntl(ngx_processes[s].channel[0], F_SETFD, FD_CLOEXEC) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "fcntl(FD_CLOEXEC) failed while spawning \"%s\"",
                           name);
            ngx_close_channel(ngx_processes[s].channel, cycle->log);
            return NGX_INVALID_PID;
        }

        
        /* 同上，这里意思是当Worker子进程执行了exec()调用后，关闭socket */  
        if (fcntl(ngx_processes[s].channel[1], F_SETFD, FD_CLOEXEC) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "fcntl(FD_CLOEXEC) failed while spawning \"%s\"",
                           name);
            ngx_close_channel(ngx_processes[s].channel, cycle->log);
            return NGX_INVALID_PID;
        }
#endif

    child = fork();
    if (child > 0) {
        sclose(fd[1]);
        proc_desc->proc_fd = fd[0];

        while (recv(fd[0], &proc_desc->proc_pid, 
            sizeof(pid_t), 0) != sizeof(pid_t));

        process_wait_child_termination(child);

        printf("Parent recv real %s pid(%d).\n",  proc_desc->proc_name, proc_desc->proc_pid);

        return proc_desc->proc_pid;	

    } else if (0 == child) {

            printf("Child name %s pid(%d).\n",  proc_desc->proc_name, getpid());

            sclose(fd[0]);

            /* set files mask */
            umask(0);
            if (setsid() == -1) exit(0);

            signal(SIGHUP, SIG_IGN);
            signal(SIGTTOU, SIG_IGN); //忽略后台进程写控制终端信号
            signal(SIGTTIN, SIG_IGN); //忽略后台进程读控制终端信号
            signal(SIGTSTP, SIG_IGN); //忽略终端挂起

            process_spwan_grandson(fd[1], proc_desc);

        } else {
            perror("fork error\n");  
        }

        return 0;
}

