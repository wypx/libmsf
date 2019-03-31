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

#include <msf_event.h>
#include <msf_miniheap.h>
#include <msf_atomic.h>

#define MSF_MOD_EVENT "EVENT"
#define MSF_EVENT_LOG(level, ...) \
    log_write(level, MSF_MOD_EVENT, __func__, __FILE__, __LINE__, __VA_ARGS__)

struct min_heap g_msf_min_heap;

extern const struct msf_event_ops epollops;

s32 msf_timer_process(void);
void* msf_timer_loop(void *param);

s32 msf_timer_init(void) {

    s32 rc = -1;
    pthread_t tm_tid;

    min_heap_ctor_(&g_msf_min_heap);

    rc = pthread_spawn(&tm_tid, (void*)msf_timer_loop, NULL);
    if (rc < 0) {
        MSF_EVENT_LOG(DBG_ERROR, 
            "Timer thread create failed, ret(%d), errno(%d).", rc, errno);
        return -1;
    }
    return 0;
}

void msf_timer_destroy(void) {
    u32 i;
    for (i = 0; i < g_msf_min_heap.n; i++) {
        sfree(g_msf_min_heap.p[i]);
    }
    min_heap_dtor_(&g_msf_min_heap);
}

s32 msf_timer_add(u32 timer_id, s32 interval, s32 (*fun)(void*), void *arg,
        s32 flag /* = CYCLE_TIMER */, s32 exe_num /* =  0 */)
{
    struct timeval now;
    struct msf_event *ev = NULL;

    ev = MSF_NEW(struct msf_event, 1);
    if (NULL == ev) {
        MSF_EVENT_LOG(DBG_ERROR, "Msf fail to new one event, errno(%d).", errno);
        return -1;
    }

    msf_memzero(ev, sizeof(struct msf_event));

    ev->ev_cbs = MSF_NEW(struct msf_event_cbs, 1);
    if (!ev->ev_cbs) {
        sfree(ev);
        return -1;
    }

    min_heap_elem_init_(ev);

    MSF_GETIMEOFDAY(&now);

    msf_memory_barrier();

    ev->ev_interval.tv_sec = interval / 1000;
    ev->ev_interval.tv_usec = (interval % 1000) * 1000;
    MSF_TIMERADD(&now, &(ev->ev_interval), &(ev->ev_timeout));
    ev->ev_flags = flag;
    ev->ev_cbs->timer_cbs  = fun;
    ev->ev_cbs->args = arg;
    ev->ev_exe_num = exe_num;
    ev->timer_id = timer_id;
 
    min_heap_push_(&g_msf_min_heap, ev);
 
    return ev->timer_id;
}

s32 msf_timer_remove(u32 timer_id) {
    u32 i = 0;
    struct msf_event * e = NULL;
    
    for (i = 0; i < g_msf_min_heap.n; i++) {
        if (timer_id == g_msf_min_heap.p[i]->timer_id) {
            e = g_msf_min_heap.p[i];
            min_heap_erase_(&g_msf_min_heap, g_msf_min_heap.p[i]);
            sfree(e);
            return 0;
        }
    }
    return -1;
}

s32 msf_timer_process(void) {
    struct msf_event *ev;
    struct timeval now;
    s32 ret = 0;

    while ((ev = min_heap_top_(&g_msf_min_heap)) != NULL) {

        MSF_GETIMEOFDAY(&now);
        if (MSF_TIMERCMP(&now, &(ev->ev_timeout), < ))
            break;
 
        min_heap_pop_(&g_msf_min_heap);
        ret = ev->ev_cbs->timer_cbs(ev->ev_cbs->args);
        //kill timer 
        if (ret == 0) {
            ev->ev_flags = LIMIT_TIMER;
            ev->ev_exe_num = 0;
        }
 
        if (ev->ev_flags == CYCLE_TIMER
           || (ev->ev_flags == LIMIT_TIMER && --ev->ev_exe_num > 0)) {
            MSF_TIMERADD(&(ev->ev_timeout), &(ev->ev_interval), &(ev->ev_timeout));
            min_heap_push_(&g_msf_min_heap, ev);
        } else {
            sfree(ev);
        }
    }

    return 0;
}


int stop_server = 0;

void* msf_timer_loop(void *param) {
    s32 rc = -1;
    s32 epfd = -1;
    struct msf_event *ev = NULL;
    struct epoll_event events[EPOLLEVENTS];
    
    epfd = msf_epoll_create();
    if (epfd < 0) {
        MSF_EVENT_LOG(DBG_ERROR, "Create epfd faild.");
        return NULL;
    }
    
    struct timeval now;
    struct timeval tv;
    struct timeval *tvp = NULL;
    long long ms = 0;
    
    while (stop_server == 0 ) {
        if ((ev = min_heap_top_(&g_msf_min_heap)) != NULL) {

            MSF_GETIMEOFDAY(&now)

            tvp = &tv;
            /* How many milliseconds we need to wait for the next
             * time event to fire? */
            ms = (ev->ev_timeout.tv_sec - now.tv_sec) * 1000 +
                    (ev->ev_timeout.tv_usec - now.tv_usec)/1000;
 
            if (ms > 0) {
                tvp->tv_sec = ms / 1000;
                tvp->tv_usec = (ms % 1000) * 1000;
            } else {
                tvp->tv_sec = 0;
                tvp->tv_usec = 0;
            }
 
            MSF_GETIMEOFDAY(&now)
 
            tv.tv_sec = ev->ev_timeout.tv_sec - now.tv_sec;;
            tv.tv_usec = ev->ev_timeout.tv_usec - now.tv_usec;
            if ( tv.tv_usec < 0 )
            {
                tv.tv_usec += 1000000;
                tv.tv_sec--;
                MSF_EVENT_LOG(DBG_ERROR, "tv.tv_usec < 0.");
            }
            tvp = &tv;
           
        }
        else
        {
            tvp = NULL;
            MSF_EVENT_LOG(DBG_ERROR, "tvp == NULL.");
        }
        if (tvp == NULL)
        {
            rc = epoll_wait(epfd, events, EPOLLEVENTS, 2000);
        }
        else
        {
            MSF_EVENT_LOG(DBG_ERROR, "timer_wait:%ld.", tvp->tv_sec*1000 + tvp->tv_usec/1000);//ms
            rc = epoll_wait(epfd, events, EPOLLEVENTS, tvp->tv_sec*1000 + tvp->tv_usec/1000);//ms
        }
 
        msf_timer_process();
    }
    sclose(epfd);
    return NULL;
}

#include <signal.h>


/////////////////////////////////////////////
static void signal_exit_func(int signo)
{
    MSF_EVENT_LOG(DBG_ERROR, "exit signo is %d.", signo);
    stop_server = 1;
}

static void signal_exit_handler()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_exit_func;
    sigaction(SIGINT, &sa, NULL);//当按下ctrl+c时，它的效果就是发送SIGINT信号
    sigaction(SIGTERM, &sa, NULL);//kill pid
    sigaction(SIGQUIT, &sa, NULL);//ctrl+\代表退出SIGQUIT
 
    //SIGSTOP和SIGKILL信号是不可捕获的,所以下面两句话写了等于没有写
    sigaction(SIGKILL, &sa, NULL);//kill -9 pid
    sigaction(SIGSTOP, &sa, NULL);//ctrl+z代表停止
 
    //#define    SIGTERM        15
    //#define    SIGKILL        9
    //kill和kill -9，两个命令在linux中都有杀死进程的效果，然而两命令的执行过程却大有不同，在程序中如果用错了，可能会造成莫名其妙的现象。
    //执行kill pid命令，系统会发送一个SIGTERM信号给对应的程序。
    //执行kill -9 pid命令，系统给对应程序发送的信号是SIGKILL，即exit。exit信号不会被系统阻塞，所以kill -9能顺利杀掉进程。
}

static const struct msf_event_ops *eventops[] = {
    &epollops,
    NULL
};


struct msf_event *msf_event_create(s32 fd,
        void (*read_cbs)(void *),
        void (*write_cbs)( void *),
        void (*error_cbs)(void *),
        void *args) {
    
    struct msf_event *ev = NULL;

    ev = MSF_NEW(struct msf_event , 1);
    if (!ev) {
        MSF_EVENT_LOG(DBG_ERROR, "Failed to new event, errno(%d).", errno);
        return NULL;
    }
    
    msf_memzero(ev, sizeof(struct msf_event));
    
    ev->ev_cbs = MSF_NEW(struct msf_event_cbs, 1);
    if (!ev->ev_cbs) {
        MSF_EVENT_LOG(DBG_ERROR, "Failed to new event cbs, errno(%d).", errno);
        sfree(ev);
        return NULL;
    }
    msf_memzero(ev->ev_cbs, sizeof(struct msf_event_cbs));
    
    ev->ev_cbs->read_cbs = read_cbs;
    ev->ev_cbs->write_cbs = write_cbs;
    ev->ev_cbs->error_cbs = error_cbs ;
    ev->ev_cbs->args = args;
    if (read_cbs) {
        ev->ev_flags |= MSF_EVENT_READ;
    }
    if (write_cbs) {
        ev->ev_flags |= MSF_EVENT_WRITE;
    }
    if (error_cbs) {
        ev->ev_flags |= MSF_EVENT_ERROR;
    }

    ev->ev_fd = fd;

    return ev;
}

void msf_event_destroy(struct msf_event *ev) {
    if (unlikely(!ev))
        return;
    
    sfree(ev->ev_cbs);
    sfree(ev);
}

s32 msf_event_add(struct msf_event_base *eb, struct msf_event *ev) {

    if (unlikely(!eb || !eb->ev_ops || !ev)) {
        MSF_EVENT_LOG(DBG_ERROR, "%s:%d paraments is NULL.", __func__, __LINE__);
        return -1;
    }
    return eb->ev_ops->add(eb, ev);
}

s32 msf_event_del(struct msf_event_base *eb, struct msf_event *ev) {
    if (unlikely(!eb || !eb->ev_ops || !ev)) {
        MSF_EVENT_LOG(DBG_ERROR, "%s:%d paraments is NULL.", __func__, __LINE__);
        return -1;
    }
    return eb->ev_ops->del(eb, ev);
}

struct msf_event_base *msf_event_base_create(void) {

    s32 i;
    s32 fds[2];
    struct msf_event_base *eb = NULL;
    
    if (pipe(fds)) {
        perror("pipe failed");
        return NULL;
    }

    msf_socket_nonblocking(fds[0]);

    eb = MSF_NEW(struct msf_event_base, 1);
    if (!eb) {
        MSF_EVENT_LOG(DBG_ERROR, "New event_base failed.");
        sclose(fds[0]);
        sclose(fds[1]);
        return NULL;
    }

    msf_memzero(eb, sizeof(struct msf_event_base));

    for (i = 0; eventops[i]; i++) {
        eb->ctx = eventops[i]->init();
        eb->ev_ops = eventops[i];
    }
    eb->loop = true;
    eb->rfd = fds[0];
    eb->wfd = fds[1];

    return eb;
}

void msf_event_base_destroy(struct msf_event_base *eb) {
    if (unlikely(!eb)) {
        return;
    }
    
    msf_event_base_loop_break(eb);
    sclose(eb->rfd);
    sclose(eb->wfd);
    eb->ev_ops->deinit(eb->ctx);
    sfree(eb);
}
s32 msf_event_base_loop(struct msf_event_base *eb) {

    s32 ret;
    const struct msf_event_ops *ev_ops = eb->ev_ops;

    while (eb->loop) {
        ret = ev_ops->dispatch(eb, NULL);
        if (ret == -1) {
            MSF_EVENT_LOG(DBG_ERROR, "Dispatch event list failed.");
            return -1;
        }
    }

    return 0;
}

void msf_event_base_loop_break(struct msf_event_base *eb) {
    s8 buf[1];
    buf[0] = false;
    eb->loop = 0;
    if (1 != write(eb->wfd, buf, 1)) {
        perror("write error");
    }
}

s32 msf_event_base_wait(struct msf_event_base *eb) {
    return eb->ev_ops->dispatch(eb, NULL);
}

void msf_event_base_signal(struct msf_event_base *eb) {
    s8 buf[1];
    buf[0] = false;
    if (1 != write(eb->wfd, buf, 1)) {
        perror("write pipe error");
    }
}

