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

#include <poll.h>

#define MSF_MOD_POLL "MSF_POLL"
#define MSF_POLL_LOG(level, ...) \
    msf_log_write(level, MSF_MOD_POLL, MSF_FUNC_FILE_LINE, __VA_ARGS__)

    
#define POLL_MAX_FD                 (1024)
#define MAX_SECONDS_IN_MSEC_LONG    (((LONG_MAX) - 999) / 1000)

struct poll_ctx {
    nfds_t          nfds;
    s32             event_count;
    struct pollfd   *fds;
};

static void *poll_init(void)
{
    struct poll_ctx *pc;
    struct pollfd *fds;
    pc = (struct poll_ctx *)calloc(1, sizeof(struct poll_ctx));
    if (!pc) {
        MSF_POLL_LOG(DBG_ERROR, "malloc poll_ctx failed.");
        return NULL;
    }
    fds = (struct pollfd *)calloc(POLL_MAX_FD, sizeof(struct pollfd));
    if (!fds) {
        MSF_POLL_LOG(DBG_ERROR, "malloc pollfd failed!.");
        return NULL;
    }
    pc->fds = fds;

    return pc;
}

static void poll_deinit(void *ctx)
{
    struct poll_ctx *pc = (struct poll_ctx *)ctx;
    if (!ctx) {
        return;
    }
    sfree(pc->fds);
    sfree(pc);
}

static s32 poll_add(struct msf_event_base *eb, struct msf_event *e)
{
    struct poll_ctx *pc = (struct poll_ctx *)eb->ctx;

    pc->fds[0].fd = e->ev_fd;

    if (e->ev_flags & MSF_EVENT_READ)
        pc->fds[0].events |= POLLIN;
    if (e->ev_flags & MSF_EVENT_WRITE)
        pc->fds[0].events |= POLLOUT;
    if (e->ev_flags & MSF_EVENT_EXCEPT)
        pc->fds[0].events |= POLLERR;

    return 0;
}
static s32 poll_del(struct msf_event_base *eb, struct msf_event *e)
{
//    struct poll_ctx *pc = eb->base;

    return 0;
}
static s32 poll_dispatch(struct msf_event_base *eb, struct timeval *tv)
{
    struct poll_ctx *pc = (struct poll_ctx *)eb->ctx;
    s32 i, n;
    s32 flags;
    int timeout = -1;

    if (tv != NULL) {
        if (tv->tv_usec > 1000000 || tv->tv_sec > MAX_SECONDS_IN_MSEC_LONG)
            timeout = -1;
        else
            timeout = (tv->tv_sec * 1000) + ((tv->tv_usec + 999) / 1000);
    } else {
        timeout = -1;
    }

    n = poll(pc->fds, pc->nfds, timeout);
    if (-1 == n) {
        MSF_POLL_LOG(DBG_ERROR, "errno=%d %s.", errno, strerror(errno));
        return -1;
    }
    if (0 == n) {
        MSF_POLL_LOG(DBG_ERROR, "poll timeout.");
        return 0;
    }
    for (i = 0; i < n; i++) {
        if (pc->fds[i].revents & POLLIN)
            flags |= MSF_EVENT_READ;
        if (pc->fds[i].revents & POLLOUT)
            flags |= MSF_EVENT_WRITE;
        if (pc->fds[i].revents & (POLLERR|POLLHUP|POLLNVAL))
            flags |= MSF_EVENT_EXCEPT;
        pc->fds[i].revents = 0;
    }
    return 0;
}

struct msf_event_ops pollops = {
    .init     = poll_init,
    .deinit   = poll_deinit,
    .add      = poll_add,
    .del      = poll_del,
    .dispatch = poll_dispatch,
};

