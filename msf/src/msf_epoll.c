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

#include <msf_network.h>
#include <msf_event.h>

#define MSF_MOD_EPOLL "MSF_EPOLL"
#define MSF_EPOLL_LOG(level, ...) \
    log_write(level, MSF_MOD_EPOLL, MSF_FUNC_FILE_LINE, __VA_ARGS__)


#define MAX_SECONDS_IN_MSEC_LONG 200

#define DEF_INIT_EPOLL_EVENT_NUM 32
#define DEF_ROOM_EPOLL_EVENT_NUM 16

struct epoll_ctx {
    s32 ep_fd;
    s32 ev_num;
    struct epoll_event *evs;
} __attribute__((__packed__));

/* On Linux kernels at least up to 2.6.24.4, epoll can't handle timeout
 * values bigger than (LONG_MAX - 999ULL)/HZ.  HZ in the wild can be
 * as big as 1000, and LONG_MAX can be as small as (1<<31)-1, so the
 * largest number of msec we can support here is 2147482.  Let's
 * round that down by 47 seconds.
 */
static void *epoll_init(void) {

    struct epoll_ctx *ep_ctx = NULL;

    ep_ctx = MSF_NEW(struct epoll_ctx, 1);
    if (!ep_ctx) {
        MSF_EPOLL_LOG(DBG_ERROR, "Failed to new epoll context, errno(%d).\n", errno);
        return NULL;
    }
    
    msf_memzero(ep_ctx, sizeof(struct epoll_ctx));
    
    ep_ctx->ep_fd = msf_epoll_create();
    if (ep_ctx->ep_fd < 0) {
        sfree(ep_ctx);
        return NULL;
    }

    ep_ctx->evs = MSF_NEW(struct epoll_event, DEF_INIT_EPOLL_EVENT_NUM);
    if (ep_ctx->ep_fd < 0) {
        sclose(ep_ctx->ep_fd);
        sfree(ep_ctx);
        return NULL;
    }
    
    return ep_ctx;
}

static void epoll_deinit(void *ctx) {
    struct epoll_ctx *ep_ctx = (struct epoll_ctx *)ctx;
    if (!ctx) {
        return;
    }
    sclose(ep_ctx->ep_fd);
    sfree(ep_ctx->evs);
    sfree(ep_ctx);
}

static s32 epoll_add(struct msf_event_base *eb, struct msf_event *ev)
{
    struct epoll_ctx *ep_ctx = (struct epoll_ctx *)eb->ctx;

    s16 events = 0;
    
    if (ev->ev_flags & MSF_EVENT_READ)
        events |= EPOLLIN;
    if (ev->ev_flags & MSF_EVENT_WRITE)
        events  |= EPOLLOUT;
    if (ev->ev_flags & MSF_EVENT_ERROR)
       events |= EPOLLERR;
    if (ev->ev_flags & MSF_EVENT_ET)
        events |= EPOLLET;
    if (ev->ev_flags & MSF_EVENT_CLOSED)
        events |= EPOLLRDHUP;
    if (ev->ev_flags & MSF_EVENT_FINALIZE)
        events |= EPOLLHUP;
    if ( ev->ev_flags & MSF_EVENT_PERSIST)
        events |= EPOLLONESHOT;

    ep_ctx->ev_num++;

    if (msf_add_event(ep_ctx->ep_fd, ev->ev_fd, events, ev) < 0) {
        return -1;
    }
    return 0;
}

static s32 epoll_mod(struct msf_event_base *eb, struct msf_event *ev) {

    struct epoll_ctx *ep_ctx = (struct epoll_ctx *)eb->ctx;

    u16 events = 0;

    if (ev->ev_flags & MSF_EVENT_READ)
       events |= EPOLLIN;
    if (ev->ev_flags & MSF_EVENT_WRITE)
       events  |= EPOLLOUT;
    if (ev->ev_flags & MSF_EVENT_ERROR)
      events |= EPOLLERR;
    if (ev->ev_flags & MSF_EVENT_ET)
       events |= EPOLLET;
    if (ev->ev_flags & MSF_EVENT_ERROR)
       events |= EPOLLRDHUP;
    if (ev->ev_flags & MSF_EVENT_ERROR)
       events |= EPOLLHUP;

    if (msf_mod_event(ep_ctx->ep_fd, ev->ev_fd, events) < 0) {
        return -1;
    }
    return 0;
}


static s32 epoll_del(struct msf_event_base *eb, struct msf_event *ev) {
    struct epoll_ctx *ep_ctx = (struct epoll_ctx *)eb->ctx;

    ep_ctx->ev_num--;
    
    if (msf_del_event(ep_ctx->ep_fd, ev->ev_fd) < 0) {
        return -1;
    }
    return 0;
}

static s32 epoll_dispatch(struct msf_event_base *eb, struct timeval *tv) {

    struct epoll_ctx *ep_ctx = (struct epoll_ctx *)eb->ctx;
    struct epoll_event *events = ep_ctx->evs;
    s32 idx, nfds = 0;
    s32  timeout = -1;
    struct msf_event *ev = NULL;
    s32 ev_what = 0;
    
    if (tv != NULL) {
        if (tv->tv_usec > 1000000 || tv->tv_sec > MAX_SECONDS_IN_MSEC_LONG)
            timeout = -1;
        else
            timeout = (tv->tv_sec * 1000) + ((tv->tv_usec + 999) / 1000);
    } else {
        timeout = -1;
    }
    
    nfds = epoll_wait(ep_ctx->ep_fd, events, DEF_INIT_EPOLL_EVENT_NUM, timeout);
    if (unlikely(nfds < 0)) {
        MSF_EPOLL_LOG(DBG_ERROR, "Epoll wait fail, errno(%d: %s).\n", 
                errno, strerror(errno));
        if (errno != EINTR) {
            return -1;
        }
        return 0;
    } else if (0 == nfds) {
        MSF_EPOLL_LOG(DBG_ERROR, "Epoll wait timeout(%d).\n", timeout);
        return 0;
    }

    for (idx = 0; idx < nfds; idx++) {
        /* 
        * Notice:
        * Epoll event struct's "data" is a union,
        * one of data.fd and data.ptr is valid.
        * refer:
        * https://blog.csdn.net/sun_z_x/article/details/22581411
        */
        ev = (struct msf_event *)events[idx].data.ptr;
         if (unlikely(!ev)) 
            continue;

        if (unlikely(events[idx].events & (EPOLLHUP | EPOLLERR))) {
            if (ev->ev_cbs->error_cbs) {
                ev->ev_cbs->error_cbs(ev->ev_cbs->args);
            } else {
                MSF_EPOLL_LOG(DBG_ERROR, "Event error cb is valid.");
                if (ev->ev_cbs->read_cbs) {
                    ev->ev_cbs->read_cbs(ev->ev_cbs->args);
                } else {
                    MSF_EPOLL_LOG(DBG_ERROR, "Event read cb is valid.");
                }
            }
        } else {
            if (events[idx].events & EPOLLIN) {
                if (ev->ev_cbs->read_cbs) {
                    ev->ev_cbs->read_cbs(ev->ev_cbs->args);
                } else {
                    MSF_EPOLL_LOG(DBG_ERROR, "Event read cb is valid.");
                }
            }

            if (events[idx].events & EPOLLOUT) {
                if (ev->ev_cbs->write_cbs) {
                    ev->ev_cbs->write_cbs(ev->ev_cbs->args);
                } else {
                    MSF_EPOLL_LOG(DBG_ERROR, "Event write cb is valid.");
                }
            }

            if (events[idx].events & EPOLLRDHUP) {
                if (ev->ev_cbs->error_cbs) {
                    ev->ev_cbs->error_cbs(ev->ev_cbs->args);
                } else {
                    MSF_EPOLL_LOG(DBG_ERROR, "Event error cb is valid.");
                }
            }
        }
    }
    return 0;
}

struct msf_event_ops epollops = {
    .init     = epoll_init,
    .deinit   = epoll_deinit,
    .add      = epoll_add,
    .del      = epoll_del,
    .dispatch = epoll_dispatch,
};

