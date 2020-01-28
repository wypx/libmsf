/*
 * NAME
 *	libaio-oracle.c - SUSv2 semantics for Linux asynchronous I/O
 *
 * AUTHOR
 * 	Joel Becker <joel.becker@oracle.com>
 *
 * DESCRIPTION
 *      This file contains implementations of the SUSv2 asynchronous
 *      I/O routines based on the Linux kernel API from Red Hat.
 *
 * MODIFIED   (YYYY/MM/DD)
 *      2003/06/18 - Joel Becker <joel.becker@oracle.com>
 *              Added aio_cancel() bits and aio_poll().
 *      2002/09/13 - Joel Becker <joel.becker@oracle.com>
 *              Fixed fork() bug; added aio_cancel() stub
 *      2002/08/07 - Joel Becker <joel.becker@oracle.com>
 *              Massaged for public release.
 *	2002/01/29 - Joel Becker <joel.becker@oracle.com>
 *		Initial merge of libaio-oracle
 *
 * Copyright (c) 2002,2003 Oracle Corporation.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License, version 2 as published by the Free Software Foundation.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have recieved a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */


#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#ifdef NEED_AIO_DATA  /* Red Hat flavoured libaio */
#define aio_data data
#define DATA_FROM_REQ(r) ((void *)(r))
#define REQ_FROM_DATA(d) ((AIORequest *)(d))
#else
#define DATA_FROM_REQ(r) ((__uint64_t)(unsigned long)(r))
#define REQ_FROM_DATA(d) ((AIORequest *)(unsigned long)(d))
#endif  /* NEED_AIO_DATA */

#include <libaio.h>

#include "libaio-oracle.h"


/*
 * Defines
 */

#define DEFAULT_QUEUE_SIZE 0x400  /* arbitrary */

#ifdef DEBUG_AIOR
#define dprintf(fmt,arg...) \
        do { if (debug_aior) { fprintf(debug_file, fmt,##arg); fflush(debug_file); } } while (0)
#else
#define dprintf(fmt,arg...)
#endif


/*
 * Typedefs
 */
typedef struct _AIORequest AIORequest;
typedef struct _AIORequestQueue AIORequestQueue;



/*
 * Enumerations
 */
enum
{
    AIOR_UNUSED = 0,            /* Unused control block */
    AIOR_IN_PROGRESS = 0xbeef,  /* In progress I/O */
    AIOR_COMPLETE = 0xdead,     /* Completed I/O */
    AIOR_COMPLETE_FREE = 0xbead /* Complete, unattached from request */
};



/*
 * Structures
 */
struct _AIORequest
{
    struct iocb iocb;                /* Control block sent to kernel */
    struct aiocb64 *aiocb;      /* User's control block */
    AIORequest *next;           /* Next in request list */
    AIORequest *prev;           /* Previous in request list */
};


struct _AIORequestQueue
{
    pid_t pid;                  /* Current PID */
    io_context_t ctxt;          /* Kernel IO context */
    unsigned long max_requests; /* Maxinum number of requests */
    AIORequest *requests;       /* Allocated requests */
    AIORequest *complete;       /* List of complete requests */
    AIORequest *current;        /* List of outstanding requests */
    AIORequest *free;           /* List of free requests */
};


/* Global for the library */
static AIORequestQueue *r_queue = NULL;

#ifdef DEBUG_AIOR
/* Global debug items */
static int debug_aior = 0;
static FILE *debug_file;
#endif  /* DEBUG_AIOR */



/*
 * Prototypes
 */
static AIORequestQueue *aior_new_request_queue(int max_requests);
static int aior_rw(int op, struct aiocb64 * const aiocbp[], int nent);
static long aior_get_events(struct timespec *timeout);



/*
 * Inline functions
 */
static inline AIORequest *aior_get_request(AIORequestQueue *queue)
{
    AIORequest *req;

    req = queue->free;
    if (req != NULL)
    {
        queue->free = req->next;
        if (queue->free != NULL)
            queue->free->prev = NULL;

        req->next = queue->current;
        if (queue->current != NULL)
            queue->current->prev = req;
        queue->current = req;
        
        return(req);
    }

    return(NULL);
}  /* aior_get_request() */


static inline void aior_complete_request(AIORequestQueue *queue,
                                         AIORequest *req)
{
    AIORequest *r_prev, *r_next;

    r_prev = req->prev;
    r_next = req->next;
    if (r_next != NULL)
        r_next->prev = r_prev;

    if (r_prev != NULL)
        r_prev->next = r_next;
    else
        queue->current = r_next;

    req->prev = NULL;
    req->next = queue->complete;
    if (queue->complete != NULL)
        queue->complete->prev = req;
    queue->complete = req;
}  /* aior_complete_request() */


static inline void aior_put_request(AIORequestQueue *queue,
                                    AIORequest *req)
{
    AIORequest *r_prev, *r_next;

    if (req->aiocb == NULL)
    {
        dprintf("aior_put_request: Invalid request being put, aiocb == NULL\n");
        return;
    }

    r_prev = req->prev;
    r_next = req->next;
    if (r_next != NULL)
        r_next->prev = r_prev;

    if (r_prev != NULL)
        r_prev->next = r_next;
    else if (req->aiocb->__aio_key == AIOR_COMPLETE)
        queue->complete = r_next;
    else if (req->aiocb->__aio_key == AIOR_IN_PROGRESS)
        queue->current = r_next;
    else
        dprintf("Invalid __aio_key during put: 0x%lx\n",
                req->aiocb->__aio_key);

    /* Clear it */
    req->aiocb = NULL;
    memset(&(req->iocb), 0, sizeof(struct iocb));

    req->prev = NULL;
    req->next = queue->free;
    if (queue->free != NULL)
        queue->free->prev = req;
    queue->free = req;
}  /* aior_put_request() */


#ifdef DEBUG_AIOR
static inline void aior_checkpoint_queue(AIORequestQueue *queue)
{
    int count, c_cur, c_com, c_free;
    AIORequest *req;

    if (debug_aior == 0)
        return;

    for (c_com = 0, req = queue->complete; req != NULL; c_com++, req = req->next);
    for (c_cur = 0, req = queue->current; req != NULL; c_cur++, req = req->next);
    for (c_free = 0, req = queue->free; req != NULL; c_free++, req = req->next);
    count = c_cur + c_com + c_free;
    dprintf("aior_checkpoint_queue: Total: %d, Complete: %d, Current: %d, Free: %d\n",
            count, c_com, c_cur, c_free);
}


static inline void aior_dump_aiocb(struct aiocb *cb)
{
    int i;
    FILE *f;
    char filename[PATH_MAX];
    static unsigned int dump_count[256];
    static int initialized = 0;

    if (debug_aior == 0)
        return;

    if (!initialized)
    {
        initialized = 1;
        memset(dump_count, 0, sizeof(dump_count));
    }

    sprintf(filename, "/tmp/aio-%d/%s-%d-%d.log", getpid(),
            cb->aio_lio_opcode == LIO_READ ? "read" : "write",
            cb->aio_fildes, dump_count[cb->aio_fildes]++);

    f = fopen(filename, "a");
    if (!f)
        return;
    fprintf(f, "aiocb at %p - %s buf at %p, count %d, offset %lld\n",
            cb, cb->aio_lio_opcode == LIO_READ ? "read to" : "write",
            cb->aio_buf, cb->aio_nbytes, cb->aio_offset);
    if (cb->aio_lio_opcode == LIO_WRITE)
    {
        for (i = 0; i < cb->aio_nbytes; i++)
        {
            if ((i % 16) == 0)
                fprintf(f, "\n%08X ", i);
            else if ((i % 8) == 0)
                fprintf(f, " ");
            fprintf(f, "%02hhX ", ((char *)cb->aio_buf)[i]);
        }
    }
    fclose(f);
}


static inline void resubmit_info(struct aiocb64 *aiocbp)
{
    int done = 0;
    AIORequest *req;

    dprintf("resubmit_info: aiocb at %p resubmitted!\n"
            "resubmit_info: buf = %p, size = %d, offset = %lld\n",
            aiocbp, aiocbp->aio_buf,
            aiocbp->aio_nbytes, aiocbp->aio_offset);

    dprintf("resubmit_info: checking opcode... ");
    switch (aiocbp->aio_lio_opcode)
    {
        case LIO_READ:
            dprintf("LIO_READ\n");
            break;
        case LIO_WRITE:
            dprintf("LIO_WRITE\n");
            break;
        default:
            dprintf("Error - invalid opcode: %d\n",
                    aiocbp->aio_lio_opcode);
    }

    dprintf("resubmit_info: Looking on current list... ");
    for (req = r_queue->current; req != NULL; req = req->next)
    {
        if (req->aiocb == aiocbp)
        {
            done = 1;
            dprintf("found - ");
            if (aiocbp->__aio_data == req)
            {
                dprintf("matches\n");
            }
            else
            {
                dprintf("aiocbp->__aio_data incorrect!\n");
            }
            break;
        }
    }
    if (req == NULL)
        dprintf("not found\n");

    dprintf("resubmit_info: Looking on complete list... ");
    for (req = r_queue->complete; req != NULL; req = req->next)
    {
        if (req->aiocb == aiocbp)
        {
            done = 1;
            dprintf("found - ");
            if (aiocbp->__aio_data == req)
                dprintf("matches\n");
            else
                dprintf("aiocbp->__aio_data incorrect!\n");
            break;
        }
    }
    if (req == NULL)
        dprintf("not found\n");

    if (done == 1)  /* Found already */
        return;

    dprintf("resubmit_info: Looking on free list... ");
    for (req = r_queue->free; req != NULL; req = req->next)
    {
        if (req == aiocbp->__aio_data)
        {
            dprintf("found\n");
            break;
        }
    }
    if (req == NULL)
        dprintf("not found\n");
}  /* resubmit_info() */


static inline void aior_debug_init()
{
    FILE *debug_aio_file;
    int c, rc;
    char f_name[PATH_MAX];
    char *p;

    debug_aio_file = fopen("/proc/debug/aio", "r");
    if (debug_aio_file == NULL)
    {
        p = getenv("DEBUG_AIOR");
        if ((p != NULL) && (*p != '\0'))
            debug_aior = 1;
    }
    else
    {
        c = fgetc(debug_aio_file);
        if ((c == '2') || (c == '3'))
            debug_aior = 1;
        fclose(debug_aio_file);
    }
    if (debug_aior == 0)
        return;

    sprintf(f_name, "/tmp/aio-%d", getpid());
    rc = mkdir(f_name, 0755);
    if (rc == 0)
    {
        sprintf(f_name, "/tmp/aio-%d/lib.log", getpid());
        debug_file = fopen(f_name, "w");
    }
    else
        debug_file = NULL;

    if (debug_file == NULL)
        debug_file = stderr;
}
#endif /* DEBUG_AIOR */


static AIORequestQueue *aior_new_request_queue(int max_requests)
{
    int rc, i;
    long size;
    AIORequestQueue *queue = NULL;

#ifdef DEBUG_AIOR
    aior_debug_init();
#endif /* DEBUG_AIOR */

    if (max_requests < 1)
        max_requests = 0x10000;  /* Mirrors kernel, FIXME */

    dprintf("aior_new_request_queue: Malloc queue\n");
    queue = (AIORequestQueue *)malloc(sizeof(AIORequestQueue));
    if (queue == NULL)
    {
        errno = ENOMEM;
        goto out;
    }

    memset(queue, 0, sizeof(*queue));
    queue->max_requests = max_requests;
    queue->pid = getpid();

    size = sizeof(AIORequest) * max_requests;
    dprintf("aior_new_request_queue: Malloc requests\n");
    queue->requests = (AIORequest *)malloc(size);
    if (queue->requests == NULL)
    {
        errno = ENOMEM;
        goto out_free_queue;
    }

    memset(queue->requests, 0, size);
    for (i = 0; i < max_requests; i++)
    {
        /* Not sure I like this style of pointer math */
        queue->requests[i].prev = queue->requests + i - 1;
        queue->requests[i].next = queue->requests + i + 1;
    }
    queue->requests[0].prev = NULL;
    queue->requests[max_requests - 1].next = NULL;
    queue->current = NULL;
    queue->free = queue->requests;

    dprintf("aior_new_request_queue: io_queue_init\n");
    rc = io_queue_init(max_requests, &(queue->ctxt));
    if (rc == 0)
    {
        dprintf("aior_new_request_queue: IO Context: %d\n",
                (unsigned int)queue->ctxt);
        goto out;
    }

    errno = -rc;
    free(queue->requests);

out_free_queue:
    free(queue);
    queue = NULL;

out:
    return(queue);
}  /* aior_new_request_queue() */


/*
 * int aio_init(int max_requests)
 *
 * Initialize asynchronous I/O with the given maximum number of
 * requests
 */
int aio_init(int max_requests)
{
    if (max_requests < 1)
    {
        errno = EINVAL;
        return(-1);
    }

    if ((r_queue != NULL) && (r_queue->pid != getpid()))
    {
        free(r_queue->requests);
        free(r_queue);
        r_queue = NULL;
    }

    if (r_queue == NULL)
    {
        /* Sets errno */
        r_queue = aior_new_request_queue(max_requests);
        if (r_queue == NULL)
            return(-1);
    }

    return(0);
}  /* aio_init() */


int aio_read64(struct aiocb64 *aiocbp)
{
    int rc;
    struct aiocb64 *aiocbs[] = {NULL};

    /* Sanity */
    if (aiocbp == NULL)
    {
        errno = EINVAL;
        return(-1);
    }

    aiocbs[0] = aiocbp;
    rc = aior_rw(LIO_READ, aiocbs, 1);

    return(rc > -1 ? 0 : -1);
}  /* aio_read64() */


int aio_write64(struct aiocb64 *aiocbp)
{
    int rc;
    struct aiocb64 *aiocbs[] = {NULL};

    /* Sanity */
    if (aiocbp == NULL)
    {
        errno = EINVAL;
        return(-1);
    }

    aiocbs[0] = aiocbp;
    rc = aior_rw(LIO_WRITE, aiocbs, 1);

    return(rc > -1 ? 0 : -1);
}  /* aio_write64() */


int aio_poll64(struct aiocb64 * aiocbp)
{
    int rc;
    struct aiocb64 *aiocbs[] = {NULL};

    /* Sanity */
    if (aiocbp == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    aiocbs[0] = aiocbp;
    rc = aior_rw(LIO_POLL, aiocbs, 1);

    return(rc > -1 ? 0 : -1);
}  /* aio_poll64() */


int lio_listio64(int mode, struct aiocb64 * const aiocbs[], int nent, 
                 struct sigevent *sig)
{
    int i;
    long ret;

    if (mode == LIO_WAIT)
    {
        errno = ENOSYS;
        return(-1);
    }
    else if (mode != LIO_NOWAIT)
    {
        errno = EINVAL;
        return(-1);
    }

    ret = 0;
    for (i = 0; i < nent; )
    {
        /* LIO_NOP means to examine aio_lio_opcode */
        ret = aior_rw(LIO_NOP, &(aiocbs[i]), nent - i);
        if (ret < 1)
            break;
        i += ret;
        ret = 0;
    }

    return(ret);
}  /* lio_listio64() */


static int aior_rw(int op, struct aiocb64 * const aiocbs[], int nent)
{
    int i, complete, rc, this_op;
    long ret;
    AIORequest *req;
    static struct iocb **iocbs = NULL;;

    /*
     * aio_read64() and aio_write64() pass in a single operation.
     * The operation type is specified in op.  POSIX says that
     * aio_read() and aio_write() cannot touch aio_lio_opcode, so
     * it cannot be used.
     *
     * lio_listio64() specifies operations in aio_lio_opcode, so it
     * passes LIO_NOP in op to signify this.
     *
     * If nent is more than 1, we were called from lio_listio64() and
     * op cannot be anything but LIO_NOP.
     */
    if ((nent > 1) && (op != LIO_NOP))
    {
        errno = EINVAL;
        return -1;
    }

    if (r_queue == NULL)
    {
        /* Sets errno */
        rc = aio_init(DEFAULT_QUEUE_SIZE);
        if ((rc < 0) || (r_queue == NULL))
            return(-1);
    }

    if (iocbs == NULL)
    {
        iocbs = (struct iocb **)malloc(r_queue->max_requests *
                                       sizeof(struct iocb *));
        if (iocbs == NULL)
        {
            errno = ENOMEM;
            return(-1);
        }
    }

#ifdef DEBUG_AIOR
    aior_checkpoint_queue(r_queue);
#endif /* DEBUG_AIOR */

    ret = 0;
    for (i = 0; i < nent; i++)
    {
        if (aiocbs[i] == NULL)
        {
            ret = -EINVAL;
            break;
        }
        if (aiocbs[i]->__aio_key == AIOR_COMPLETE)
        {
            /* Likely a complete request being reused */
            if (((unsigned long)(aiocbs[i]->__aio_data) <
                 (unsigned long)(r_queue->requests)) ||
                ((unsigned long)(aiocbs[i]->__aio_data) >
                 (unsigned long)(r_queue->requests + r_queue->max_requests)))
            {
                /* Garbage request pointer, likely an uncleared aiocb */
                dprintf("aior_rw: Garbage pointer: %p (req %p, max %ld)\n",
                        aiocbs[i]->__aio_data,
                        r_queue->requests,
                        r_queue->max_requests);
                aiocbs[i]->__aio_key = AIOR_UNUSED;
                aiocbs[i]->__aio_data = NULL;
            }
            else
            {
                /* Put the request back on the free list */
                dprintf("aior_rw: Reused aiocb at %p\n",
                        aiocbs[i]);
                aior_put_request(r_queue,
                                 (AIORequest *)aiocbs[i]->__aio_data);
                aiocbs[i]->__aio_key = AIOR_UNUSED;
                aiocbs[i]->__aio_data = NULL;
            }
        }
        else if (aiocbs[i]->__aio_key == AIOR_IN_PROGRESS)
        {
            if (((unsigned long)(aiocbs[i]->__aio_data) <
                 (unsigned long)(r_queue->requests)) ||
                ((unsigned long)(aiocbs[i]->__aio_data) >
                 (unsigned long)(r_queue->requests + r_queue->max_requests)))
            {
                /* Garbage request pointer, likely an uncleared aiocb */
                dprintf("aior_rw: Garbage pointer: %p (req %p, max %ld)\n",
                        aiocbs[i]->__aio_data,
                        r_queue->requests,
                        r_queue->max_requests);
                aiocbs[i]->__aio_key = AIOR_UNUSED;
                aiocbs[i]->__aio_data = NULL;
            }
            else
            {
                /* Currently queued request, undefined behavior */
#ifdef DEBUG_AIOR
                resubmit_info(aiocbs[i]);
#endif /* DEBUG_AIOR */
                ret = -EINPROGRESS;
                break;
            }
        }
        else if ((aiocbs[i]->__aio_key != AIOR_UNUSED) &&
                 (aiocbs[i]->__aio_key != AIOR_COMPLETE_FREE))
        {
            /* They were bad and didn't zero it out */
            dprintf("aior_rw: aiocb at %p has garbage __aio_key: 0x%lx\n",
                    aiocbs[i], aiocbs[i]->__aio_key);
            aiocbs[i]->__aio_key = AIOR_UNUSED;
            aiocbs[i]->__aio_data = NULL;
        }

        req = aior_get_request(r_queue);
        if (req == NULL)
        {
            dprintf("aior_rw: No requests available\n");
            ret = -EAGAIN;
            break;
        }
        dprintf("aior_rw: aiocb at %p requesting ", aiocbs[i]);
    
        req->aiocb = aiocbs[i];
        aiocbs[i]->__aio_data = req;
        aiocbs[i]->__aio_key = AIOR_IN_PROGRESS;  /* Enqueued */
        if (op == LIO_NOP)
            this_op = aiocbs[i]->aio_lio_opcode;
        else
            this_op = op;
        switch (this_op)
        {
            case LIO_READ:
                dprintf("read");
                io_prep_pread(&(req->iocb),
                              aiocbs[i]->aio_fildes,
                              aiocbs[i]->aio_buf,
                              aiocbs[i]->aio_nbytes,
                              aiocbs[i]->aio_offset);
                break;
    
            case LIO_WRITE:
                dprintf("write");
                io_prep_pwrite(&(req->iocb),
                               aiocbs[i]->aio_fildes,
                               aiocbs[i]->aio_buf,
                               aiocbs[i]->aio_nbytes,
                               aiocbs[i]->aio_offset);
                break;
    
            case LIO_POLL:
                dprintf("poll");
                io_prep_poll(&(req->iocb),
                             aiocbs[i]->aio_fildes,
                             (int)aiocbs[i]->aio_buf);
                break;

            case LIO_NOP:
                dprintf("nop");
                aior_put_request(r_queue, req);
                aiocbs[i]->__aio_key = AIOR_UNUSED;
                aiocbs[i]->__aio_data = NULL;
                continue;

            default:
                dprintf("error - Bad request: %d\n",
                        this_op);
                ret = -EINVAL;
                break;
        }
        if (ret == -1)
        {
            aior_put_request(r_queue, req);
            aiocbs[i]->__aio_key = AIOR_UNUSED;
            aiocbs[i]->__aio_data = NULL;
            break;
        }
    
        dprintf(": buf at %p\n", aiocbs[i]->aio_buf);

        req->iocb.aio_data = DATA_FROM_REQ(req);
        iocbs[i] = &(req->iocb);
#ifdef DEBUG_AIOR
        aior_dump_aiocb(aiocbs[i]);
#endif /* DEBUG_AIOR */
    }

    complete = i;

    if (complete > 0)
    {
        ret = io_submit(r_queue->ctxt, complete, iocbs);
        dprintf("aior_rw: io_submit returned %ld/%d (%d)\n",
                ret, complete, nent);
        if ((ret == -EINVAL) &&
            (r_queue != NULL) && (r_queue->pid != getpid()))
        {
            /*
             * Bad queue, likely a fork()
             * This is a really ugly cleanup, but to automagically
             * resubmit we have to copy all the requests to the 
             * new context.
             */
            AIORequestQueue *old_queue = r_queue;

            /* so aio_init() doesn't squash current requests */
            r_queue = NULL;
            rc = aio_init(DEFAULT_QUEUE_SIZE);
            if ((rc < 0) || (r_queue == NULL))
            {
                r_queue = old_queue; /* Sanity */
                ret = -errno;
            }
            else
            {
                for (i = 0; i < complete; i++)
                {
                    AIORequest *old_req =
                        REQ_FROM_DATA(iocbs[i]->aio_data);

                    req = aior_get_request(r_queue);
                    if (req == NULL)
                    {
                        dprintf("aior_rw: No requests available\n");
                        ret = -EAGAIN;
                        break;
                    }
    
                    req->aiocb = old_req->aiocb;
                    req->aiocb->__aio_data = req;
                    memcpy(&(req->iocb), &(old_req->iocb), 
                           sizeof(req->iocb));
                    req->iocb.aio_data = DATA_FROM_REQ(req);
                    iocbs[i] = &(req->iocb);
                }

                if (i != complete)
                {
                    /*
                     * Oh, heck, the new queue is smaller than
                     * the number of requests.  Clean up the
                     * remainder.  Note that EAGAIN will already
                     * be set.  Basically, assert(ret == -EAGAIN).
                     * We don't have to put_request(), because the
                     * unmigrated I/Os are on the old_queue.
                     */
                    int j;
                    for (j = i; j < complete; j++)
                    {
                        req = REQ_FROM_DATA(iocbs[j]->aio_data);
                        req->aiocb->__aio_key = AIOR_UNUSED;
                        req->aiocb->__aio_data = NULL;
                    }
                    complete = i;
                }

                free(old_queue->requests);
                free(old_queue);

                /* Yeech.  Finally */
                ret = io_submit(r_queue->ctxt, complete, iocbs);
                dprintf("aior_rw: re-io_submit returned %ld/%d (%d)\n",
                        ret, complete, nent);
            }
        }
    }

    /* Clear any that failed */
    for (i = ((ret < 0) ? 0 : ret); i < complete; i++)
    {
        dprintf("aior_rw: Failed to submit aiocb at %p\n",
                aiocbs[i]);
        aior_put_request(r_queue, aiocbs[i]->__aio_data);
        aiocbs[i]->__aio_key = AIOR_UNUSED;
        aiocbs[i]->__aio_data = NULL;
    }
    /* Flag unsubmitted */
    for (; i < nent; i++)
    {
        dprintf("aior_rw: Flagging unsubmitted aiocb at %p\n",
                aiocbs[i]);
        aiocbs[i]->__aio_key = AIOR_UNUSED;
    }

#ifdef DEBUG_AIOR
    aior_checkpoint_queue(r_queue);
#endif /* DEBUG_AIOR */

    if (ret < 0)
    {
        errno = -ret;
        ret = -1;
    }
    return(ret);
}


static long aior_get_events(struct timespec *timeout)
{
    struct io_event events[MAX_EVENTS];
    int i;
    long ret;
    AIORequest *req;

    if (r_queue == NULL)
        return(-EINVAL);

    /* Thanks for no compat defines in libaio.h */
#ifdef OLD_LIBAIO
    ret = io_getevents(r_queue->ctxt, MAX_EVENTS, events, timeout);
#else
    ret = io_getevents(r_queue->ctxt, 1, MAX_EVENTS, events, timeout);
#endif /* OLD_LIBAIO */
    if (ret < 0)
        return(ret);
    else if (ret == 0)
        return(0);

    for (i = 0; i < ret; i++)
    {
        req = REQ_FROM_DATA(events[i].data);
        req->aiocb->__error_code = events[i].res;
        if (req->aiocb->__aio_key != AIOR_IN_PROGRESS)
        {
            dprintf("aior_get_events: aiocb at %p has __aio_key %ld!\n",
                    req->aiocb, req->aiocb->__aio_key);
            continue;
        }
        req->aiocb->__aio_key = AIOR_COMPLETE;

        dprintf("aior_get_events: Collected aiocb at %p\n",
                req->aiocb);

        aior_complete_request(r_queue, req);
    }

#ifdef DEBUG_AIOR
    aior_checkpoint_queue(r_queue);
#endif /* DEBUG_AIOR */

    return(ret);
}  /* aior_get_events() */


int aio_error64(struct aiocb64 *aiocbp)
{
    struct timespec to = {0, 0};
    long ret;

    if (aiocbp == NULL)
    {
        errno = EINVAL;
        return(-1);
    }

    /* Quick check */
    if ((aiocbp->__aio_key == AIOR_COMPLETE) ||
        (aiocbp->__aio_key == AIOR_COMPLETE_FREE))
    {
        dprintf("aio_error: aiocb at %p complete\n",
                aiocbp);
        if (aiocbp->__error_code < 0)
            return(-(aiocbp->__error_code));
        else
            return(0);
    }

    /* Unsubmitted aiocb */
    if (aiocbp->__aio_key == AIOR_UNUSED)
        return(EAGAIN);

    /* Do we have an invalid __aio_key? */
    if (aiocbp->__aio_key != AIOR_IN_PROGRESS)
    {
        errno = EINVAL;
        return(-1);
    }

    ret = aior_get_events(&to);
    if (ret < 0)
    {
        errno = -ret;
        return(-1);
    }

    /*
     * AIOR_COMPLETE_FREE really shouldn't happen here, but it
     * doesn't hurt.
     */
    if ((aiocbp->__aio_key == AIOR_COMPLETE) ||
        (aiocbp->__aio_key == AIOR_COMPLETE_FREE))
    {
        dprintf("aio_error: aiocb at %p complete\n",
                aiocbp);
        if (aiocbp->__error_code < 0)
            return(-(aiocbp->__error_code));
        else
            return(0);
    }

#if 0
    errno = EINPROGRESS;
    return(-1);
#else
    return(EINPROGRESS);  /* BROKEN, but expected */
#endif
}  /* aio_error64() */


ssize_t aio_return64(struct aiocb64 *aiocbp)
{
    struct timespec to = {0, 0};
    long ret;

    if (aiocbp == NULL)
    {
        errno = EINVAL;
        return(-1);
    }

    /* Quick check */
    if (aiocbp->__aio_key == AIOR_COMPLETE)
        goto complete;
    else if (aiocbp->__aio_key == AIOR_COMPLETE_FREE)
        goto complete_free;
    else if (aiocbp->__aio_key != AIOR_IN_PROGRESS)
    {
        errno = EINVAL;
        return(-1);
    }

    ret = aior_get_events(&to);
    if (ret < 0)
    {
        errno = -ret;
        return(-1);
    }

    if (aiocbp->__aio_key == AIOR_COMPLETE)
    {
complete:
        aior_put_request(r_queue, (AIORequest *)aiocbp->__aio_data);
        aiocbp->__aio_data = NULL;
        aiocbp->__aio_key = AIOR_COMPLETE_FREE;

complete_free:
        if (aiocbp->__error_code < 0)
        {
            errno = -(aiocbp->__error_code);
            return(-1);
        }
        else
            return(aiocbp->__error_code);
    }

    return(-EINPROGRESS);  /* Undefined, really */
}  /* aio_return64() */


int aio_suspend64(const struct aiocb64 * const list[], int nent,
                  const struct timespec *timeout)
{
    struct timespec mod_timeout;
    long ret = 0;
    int done = 0, i;

    /* Get round constness */
    if (timeout)
        memcpy(&mod_timeout, timeout, sizeof(mod_timeout));

    while (done == 0)
    {
        for (i = 0; i < nent; i++)
        {
            if ((list[i] != NULL) && 
                ((list[i]->__aio_key == AIOR_COMPLETE) ||
                 (list[i]->__aio_key == AIOR_COMPLETE_FREE)))
            {
                ret = 0;
                done = 1;
                break;
            }
        }

        if (done == 0)  
        {
            /*
             * Note that this relative timeout is currently not
             * adjusted.  This means that each time around the loop
             * a full timeout is incurred.  Hopefully the kernel will
             * soon adjust the timeout when it returns.  Otherwise,
             * aio_suspend() and aio_reap() will have to do
             * gettimeofday() tricks to make it work right.
             */
            ret = aior_get_events(timeout ? &mod_timeout : NULL);
            if ((ret == -ETIMEDOUT) || (ret == 0))
            {
                errno = EAGAIN;
                ret = -1;
                done = 1;
            }
        }
    }

    return(ret);
}  /* aio_suspend64() */


int aio_reap64(struct timespec *timeout, int waitfor,
               struct aiocb *out_list[], int listsize,
               int *completed_count)
{
    long ret;
    int done = 0;
    AIORequest *req;

    if ((listsize > MAX_AIO_REAP) || (waitfor > listsize) ||
        (waitfor < 1))
    {
        errno = EINVAL;
        return(-1);
    }

    *completed_count = 0;
    ret = 0;
    while (done == 0)
    {
        while (r_queue->complete != NULL)
        {
            req = r_queue->complete;

            out_list[*completed_count] = req->aiocb;
            aior_put_request(r_queue, req);
            out_list[*completed_count]->__aio_key = AIOR_COMPLETE_FREE;
            
            (*completed_count)++;
            if (*completed_count >= waitfor)
            {
                done = 1;
                ret = 0;
            }

            if (*completed_count >= listsize)
            {
                done = 1;  /* Sanity */
                ret = 0;
                break;
            }
        }

        if ((*completed_count < waitfor) &&
            (r_queue->current == NULL))
        {
            done = 1;
            errno = E2BIG;
            ret = -1;
        }

        if (done == 0)
        {
            /* See comment about timeouts in aio_suspend() */
            ret = aior_get_events(timeout);
            if (ret <= 0)
            {
                if ((ret == 0) || (ret == -ETIMEDOUT))
                {
                    ret = -1;
                    errno = EAGAIN;
                    break;
                }
                else
                {
                    errno = -ret;
                    ret = -1;
                    break;  /* Real error, bail */
                }
            }
            else
                dprintf("aio_reap: %ld events\n", ret);
        }
    }

#ifdef  DEBUG_AIOR
    aior_checkpoint_queue(r_queue);
#endif /* DEBUG_AIOR */

    return(ret);
}  /* aio_reap64() */


#ifndef OLD_LIBAIO
int aior_cancel_one(struct aiocb64 *aiocbp)
{
    struct io_event event;
    int ret;
    AIORequest *req = (AIORequest *)aiocbp->__aio_data;

    if (!req)
    {
        errno = EINVAL;
        return -1;
    }
    
    ret = io_cancel(r_queue->ctxt,
                    &(req->iocb),
                    &event);
    if (ret < 0)
    {
        if (ret == -EAGAIN)
            return AIO_NOTCANCELED;
        errno = -ret;
        return -1;
    }

    if (req != (AIORequest *)event.data)
    {
        dprintf("aior_cancel_one: Result and request do not match!\n");
        errno = EINVAL;
        return -1;
    }

    if (req->aiocb->__aio_key != AIOR_IN_PROGRESS)
    {
        dprintf("aior_cancel_one: aiocb at %p has __aio_key %ld!\n",
                req->aiocb, req->aiocb->__aio_key);
    }

    req->aiocb->__error_code = -ECANCELED;
    req->aiocb->__aio_key = AIOR_COMPLETE;

    dprintf("aior_cancel_one: Collected aiocb at %p\n",
            req->aiocb);
    aior_complete_request(r_queue, req);

    return AIO_CANCELED;
}  /* aior_cancel_one() */
#endif  /* ! OLD_LIBAIO */


int aio_cancel64(int filedes, struct aiocb64 *aiocbp)
{
    int ret, cancelled;
    AIORequest *req;

    if (aiocbp)
    {
        if (aiocbp->aio_fildes != filedes)
        {
            errno = EINVAL;
            return -1;
        }
        
        if ((aiocbp->__aio_key == AIOR_COMPLETE) ||
            (aiocbp->__aio_key == AIOR_COMPLETE_FREE))
            return AIO_ALLDONE;

#ifdef OLD_LIBAIO
        return AIO_NOTCANCELED;
#else
        return aior_cancel_one(aiocbp);
#endif  /* OLD_LIBAIO */
    }

    ret = AIO_ALLDONE;
    cancelled = 1;
    for (req = r_queue->current; req; req = req->next)
    {
        if (req->aiocb->aio_fildes == filedes)
#ifdef OLD_LIBAIO
            return AIO_NOTCANCELED;
#else
            ret = aior_cancel_one(req->aiocb);
        if (ret < 0)
            break;
        if (ret == AIO_NOTCANCELED)
            cancelled = 0;
#endif  /* OLD_LIBAIO */
    }

    /* Don't know EBADF, sys_io_cancel() will tell us in the future */
    if (ret < 0)
    {
        errno = -ret;
        ret = -1;
    }
    else
        ret = cancelled ? AIO_CANCELED : AIO_NOTCANCELED;

    return ret;
}  /* aio_cancel() */


