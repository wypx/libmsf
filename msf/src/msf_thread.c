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

#include <msf_thread.h>
#include <msf_log.h>

#define MSF_MOD_THREAD "THREAD"
#define MSF_THREAD_LOG(level, ...) \
    log_write(level, MSF_MOD_THREAD, MSF_FUNC_FILE_LINE, __VA_ARGS__)

/*
    pthread_cond_wait必须放在pthread_mutex_lock和pthread_mutex_unlock之间,
    因为他要根据共享变量的状态来决定是否要等待，而为了不永远等待下去所以必须
    要在lock/unlock队中共享变量的状态改变必须遵守lock/unlock的规则
    pthread_cond_signal即可以放在pthread_mutex_lock和pthread_mutex_unlock之间,
    也可以放在pthread_mutex_lock和pthread_mutex_unlock之后，但是各有各缺点。

    pthread_mutex_lock
    xxxxxxx
    pthread_cond_signal
    pthread_mutex_unlock
    缺点：在某些线程的实现中,会造成等待线程从内核中唤醒（由于cond_signal)
    然后又回到内核空间（因为cond_wait返回后会有原子加锁的行为）,所以一来
    一回会有性能的问题。但是在LinuxThreads或者NPTL里面，就不会有这个问题,
    因为在Linux 线程中,有两个队列,分别是cond_wait队列和mutex_lock队列,
    cond_signal只是让线程从cond_wait队列移到mutex_lock队列,而不用返回到用户空间,
    不会有性能的损耗.所以在Linux中推荐使用这种模式.
    pthread_mutex_lock
    xxxxxxx
    pthread_mutex_unlock
    pthread_cond_signal
    优点：不会出现之前说的那个潜在的性能损耗，因为在signal之前就已经释放锁了
    缺点：如果unlock和signal之前,有个低优先级的线程正在mutex上等待的话,
    那么这个低优先级的线程就会抢占高优先级的线程（cond_wait的线程),
    而这在上面的放中间的模式下是不会出现的.
    所以在Linux下最好pthread_cond_signal放中间,但从编程规则上说,其他两种都可以
    */


/*
    pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t*mutex)函数传入的参数
    mutex用于保护条件，因为我们在调用pthread_cond_wait时,如果条件不成立我们就进入阻塞,
    但是进入阻塞这个期间,如果条件变量改变了的话,那我们就漏掉了这个条件.
    因为这个线程还没有放到等待队列上,所以调用pthread_cond_wait前要先锁互斥量,
    即调用pthread_mutex_lock(),pthread_cond_wait在把线程放进阻塞队列后,
    自动对mutex进行解锁,使得其它线程可以获得加锁的权利.
    这样其它线程才能对临界资源进行访问并在适当的时候唤醒这个阻塞的进程.
    当pthread_cond_wait返回的时候又自动给mutex加锁.
    实际上边代码的加解锁过程如下：
    / ***********pthread_cond_wait()的使用方法********** /
    pthread_mutex_lock(&qlock); / *lock* /
    pthread_cond_wait(&qready, &qlock); / *block-->unlock-->wait() return-->lock* /
    pthread_mutex_unlock(&qlock); / *unlock* /
    pthread_cond_wait内部执行流程是:进入阻塞--解锁--进入线程等待队列,
    添加满足(其他线程ngx_thread_cond_signal)后，再次加锁，然后返回
    为什么在调用该函数的时候外层要先加锁,
    原因是在pthread_cond_wait内部进入阻塞状态有个过程,
    这个过程中其他线程cond signal，本线程检测不到该信号signal,
    所以外层加锁就是让本线程进入wait线程等待队列后,
    才允许其他线程cond signal唤醒本线程，就可以避免漏掉信号
*/

#if 0
ngx_int_t
ngx_thread_mutex_create(ngx_thread_mutex_t *mtx, ngx_log_t *log)
{
    ngx_err_t            err;
    pthread_mutexattr_t  attr;

    err = pthread_mutexattr_init(&attr);
    if (err != 0) {
        ngx_log_error(NGX_LOG_EMERG, log, err,
                      "pthread_mutexattr_init() failed");
        return NGX_ERROR;
    }

    /*
    如果mutex类型是 PTHREAD_MUTEX_ERRORCHECK，那么将进行错误检查。如果一个线程企图对一个已经锁住的mutex进行relock，将返回一
    个错误。如果一个线程对未加锁的或已经unlock的mutex对象进行unlock操作，将返回一个错误。 
     */
    err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    if (err != 0) {
        ngx_log_error(NGX_LOG_EMERG, log, err,
                      "pthread_mutexattr_settype"
                      "(PTHREAD_MUTEX_ERRORCHECK) failed");
        return NGX_ERROR;
    }

    err = pthread_mutex_init(mtx, &attr);
    if (err != 0) {
        ngx_log_error(NGX_LOG_EMERG, log, err,
                      "pthread_mutex_init() failed");
        return NGX_ERROR;
    }

    err = pthread_mutexattr_destroy(&attr);
    if (err != 0) {
        ngx_log_error(NGX_LOG_ALERT, log, err,
                      "pthread_mutexattr_destroy() failed");
    }

    ngx_log_debug1(NGX_LOG_DEBUG_CORE, log, 0,
                   "pthread_mutex_init(%p)", mtx);
    return NGX_OK;
}


ngx_int_t
ngx_thread_mutex_destroy(ngx_thread_mutex_t *mtx, ngx_log_t *log)
{
    ngx_err_t  err;

    err = pthread_mutex_destroy(mtx);
    if (err != 0) {
        ngx_log_error(NGX_LOG_ALERT, log, err,
                      "pthread_mutex_destroy() failed");
        return NGX_ERROR;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_CORE, log, 0,
                   "pthread_mutex_destroy(%p)", mtx);
    return NGX_OK;
}


ngx_int_t
ngx_thread_mutex_lock(ngx_thread_mutex_t *mtx, ngx_log_t *log)
{
    ngx_err_t  err;

    ngx_log_debug1(NGX_LOG_DEBUG_CORE, log, 0,
                   "pthread_mutex_lock(%p) enter", mtx);

    err = pthread_mutex_lock(mtx);
    if (err == 0) {
        return NGX_OK;
    }

    ngx_log_error(NGX_LOG_ALERT, log, err, "pthread_mutex_lock() failed");

    return NGX_ERROR;
}


ngx_int_t
ngx_thread_mutex_unlock(ngx_thread_mutex_t *mtx, ngx_log_t *log)
{
    ngx_err_t  err;

    err = pthread_mutex_unlock(mtx);

#if 0
    ngx_time_update();
#endif

    if (err == 0) {
        ngx_log_debug1(NGX_LOG_DEBUG_CORE, log, 0,
                       "pthread_mutex_unlock(%p) exit", mtx);
        return NGX_OK;
    }

    ngx_log_error(NGX_LOG_ALERT, log, err, "pthread_mutex_unlock() failed");

    return NGX_ERROR;
}
#endif



s32 pthread_spawn(pthread_t *tid, void* (*func)(void *), void *arg) {
    s32 rc;
    pthread_attr_t thread_attr;

#ifndef WIN32
    sigset_t signal_mask;
    sigemptyset (&signal_mask);
    sigaddset (&signal_mask, SIGPIPE);
    rc = pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);
    if (rc != 0) {
        MSF_THREAD_LOG(DBG_ERROR, "block sigpipe error\n");
    } 
#endif

    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
    if (pthread_create(tid, &thread_attr, (void *)func, arg) < 0) {
        MSF_THREAD_LOG(DBG_ERROR, "pthread_create");
        return -1;
    }

    return 0;
}

