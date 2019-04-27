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

/*
实际上，上述问题的解决离不开Nginx的post事件处理机制。这个post事件是什么意思呢？它表示允许事件延后执行。Nginx设计了两个post队列，一
个是由被触发的监听连接的读事件构成的ngx_posted_accept_events队列，另一个是由普通读／写事件构成的ngx_posted_events队列。这样的post事
件可以让用户完成什么样的功能呢？
将epoll_wait产生的一批事件，分到这两个队列中,让存放着新连接事件的ngx_posted_accept_events队列优先执行，存放普通事件的ngx_posted_events队
列最后执行，这是解决“惊群”和负载均衡两个问题的关键.
如果在处理一个事件的过程中产生了另一个事件,而我们希望这个事件随后执行（不是立刻执行），
就可以把它放到post队列中。
*/

#include <msf_time.h>
#include <msf_list.h>
#include <msf_network.h>

#define EPOLLEVENTS 100
#define LIMIT_TIMER 1 /* 有限次数定时器 */
#define CYCLE_TIMER 2 /* 循环定时器 */

enum msf_event_flags {
    MSF_EVENT_TIMEOUT  = 1<<0,
    MSF_EVENT_READ     = 1<<1,
    MSF_EVENT_WRITE    = 1<<2,
    MSF_EVENT_SIGNAL   = 1<<3,
    MSF_EVENT_PERSIST  = 1<<4,
    MSF_EVENT_ET       = 1<<5,
    MSF_EVENT_FINALIZE = 1<<6,
    MSF_EVENT_CLOSED   = 1<<7,
    MSF_EVENT_ERROR    = 1<<8,
    MSF_EVENT_EXCEPT   = 1<<9,
}__attribute__((__packed__));

struct msf_event_cbs {
    void (*read_cbs)(void *arg);
    void (*write_cbs)(void *arg);
    void (*error_cbs)(void *arg);
    s32 (*timer_cbs)(void *arg);
    void *args;
}__attribute__((__packed__));

struct msf_event {
    /* for managing timeouts */
    union {
        struct list_head ev_list; /* ev_next_with_common_timeout */
        s32 min_heap_idx;
    } ev_timeout_pos;

    u32 ev_priority;
    u32 timer_id;
    struct timeval ev_interval;
    struct timeval ev_timeout;
    s32 ev_exe_num;

    struct msf_event_cbs *ev_cbs;

    s32 ev_fd;
    s32 ev_res; /* result passed to event callback */
    s32 ev_flags;
} __attribute__((__packed__));

struct msf_event_base;
struct msf_event_ops {
    void *(*init)();
    void (*deinit)(void *ctx);
    s32 (*add)(struct msf_event_base *eb, struct msf_event *ev);
    s32 (*del)(struct msf_event_base *eb, struct msf_event *ev);
    s32 (*dispatch)(struct msf_event_base *eb, struct timeval *tv);
}__attribute__((__packed__));

struct msf_event_base {
    /** Pointer to backend-specific data. */
    void *ctx;
    s32 loop;
    s32 rfd;
    s32 wfd;
    const struct msf_event_ops *ev_ops;
} __attribute__((__packed__));


s32 msf_timer_init(void);
void msf_timer_destroy(void);
s32 msf_timer_add(u32 timer_id, s32 interval, s32 (*fun)(void*), void *arg, s32 flag, s32 exe_num);
s32 msf_timer_remove(u32 timer_id);

struct msf_event *msf_event_new(s32 fd,
        void (*read_cbs)(void *),
        void (*write_cbs)(void *),
        void (*error_cbs)(void *),
        void *args);
void msf_event_free(struct msf_event *ev);
s32 msf_event_add(struct msf_event_base *eb, struct msf_event *ev);
s32 msf_event_del(struct msf_event_base *eb, struct msf_event *ev);

struct msf_event_base *msf_event_base_new(void);
void msf_event_base_free(struct msf_event_base *eb);
s32 msf_event_base_dispatch(struct msf_event_base *eb);
void msf_event_base_loopexit(struct msf_event_base *eb);
s32 msf_event_base_wait(struct msf_event_base *eb);
void msf_event_base_signal(struct msf_event_base *eb);

