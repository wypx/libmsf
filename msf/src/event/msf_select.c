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
#include <sys/select.h>

#define MSF_MOD_SELECT "MSF_SELECT"
#define MSF_SELECT_LOG(level, ...) \
    msf_log_write(level, MSF_MOD_SELECT, MSF_FUNC_FILE_LINE, __VA_ARGS__)

#define SELECT_MAX_FD	1024
    
struct select_ctx {
    s32 nfds;       /* Highest fd in fd set */
    fd_set *rfds;
    fd_set *wfds;
    fd_set *efds;
};

static void *select_init(void)
{
    struct select_ctx *sc = calloc(1, sizeof(struct select_ctx));
    if (!sc) {
        MSF_SELECT_LOG(DBG_ERROR, "malloc select_ctx failed!.");
        return NULL;
    }
    fd_set *rfds = calloc(1, sizeof(fd_set));
    fd_set *wfds = calloc(1, sizeof(fd_set));
    fd_set *efds = calloc(1, sizeof(fd_set));
    if (!rfds || !wfds || !efds) {
        MSF_SELECT_LOG(DBG_ERROR, "malloc fd_set failed.");
        return NULL;
    }
    sc->rfds = rfds;
    sc->wfds = wfds;
    sc->efds = efds;
    return sc;
}

static void select_deinit(void *ctx)
{
    struct select_ctx *sc = (struct select_ctx *)ctx;
    if (!ctx) {
        return;
    }
    sfree(sc->rfds);
    sfree(sc->wfds);
    sfree(sc->efds);
    sfree(sc);
}

static s32 select_add(struct msf_event_base *eb, struct msf_event *e)
{
    struct select_ctx *sc = (struct select_ctx *)eb->ctx;

    FD_ZERO(sc->rfds);
    FD_ZERO(sc->wfds);
    FD_ZERO(sc->efds);

    if (sc->nfds < e->ev_fd) {
        sc->nfds = e->ev_fd;
    }

    if (e->ev_flags & MSF_EVENT_READ)
        FD_SET(e->ev_fd, sc->rfds);
    if (e->ev_flags & MSF_EVENT_WRITE)
        FD_SET(e->ev_fd, sc->wfds);
    if (e->ev_flags & MSF_EVENT_EXCEPT)
        FD_SET(e->ev_fd, sc->efds);
    return 0;
}

static s32 select_del(struct msf_event_base *eb, struct msf_event *e)
{
    struct select_ctx *sc = (struct select_ctx *)eb->ctx;
    if (sc->rfds)
        FD_CLR(e->ev_fd, sc->rfds);
    if (sc->wfds)
        FD_CLR(e->ev_fd, sc->wfds);
    if (sc->efds)
        FD_CLR(e->ev_fd, sc->efds);
    return 0;
}

static s32 select_dispatch(struct msf_event_base *eb, struct timeval *tv)
{
    s32 i, n;
    s32 flags;
    struct select_ctx *sc = (struct select_ctx *)eb->ctx;
    s32 nfds = sc->nfds + 1;

    n = select(nfds, sc->rfds, sc->wfds, sc->efds, tv);
    if (-1 == n) {
        MSF_SELECT_LOG(DBG_ERROR, "errno=%d %s.", errno, strerror(errno));
        return -1;
    }
    if (0 == n) {
        MSF_SELECT_LOG(DBG_ERROR, "select timeout.");
        return 0;
    }
    for (i = 0; i < nfds; i++) {
        if (FD_ISSET(i, sc->rfds))
            flags |= MSF_EVENT_READ;
        if (FD_ISSET(i, sc->wfds))
            flags |= MSF_EVENT_WRITE;
        if (FD_ISSET(i, sc->efds))
            flags |= MSF_EVENT_EXCEPT;
    }

    return 0;
}

struct msf_event_ops selectops = {
    .init     = select_init,
    .deinit   = select_deinit,
    .add      = select_add,
    .del      = select_del,
    .dispatch = select_dispatch,
};

