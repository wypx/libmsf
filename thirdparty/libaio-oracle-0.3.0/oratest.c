/*
 * NAME
 *	oratest.c - Testing datafile creation
 *
 * AUTHOR
 * 	Joel Becker <joel.becker@oracle.com>
 *
 * DESCRIPTION
 *      This test writes large data files via differing syncronous and
 *      asynchronous methods.  It is intended to loosely simulate the
 *      data file creation steps of a new instance.
 *
 * MODIFIED   (YYYY/MM/DD)
 *      2002/08/07 - Joel Becker <joel.becker@oracle.com>
 *              Modified for public release.
 *	2002/01/25 - Joel Becker <joel.becker@oracle.com>
 *		Initial header description.
 *
 * Copyright (c) 2002 Oracle Corporation.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License, version 2 as published by the Free Software Foundation.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have recieved a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "libaio-oracle.h"


/*
 * Defines
 */
#ifndef PAGE_SIZE
#define PAGE_SIZE getpagesize()
#endif

#define WRITE_FILE "write_test"
#define SUSPEND_FILE "suspend_test"
#define REAP_FILE "reap_test"
#define SEEK_SIZE 0x70000000  /* Arbitrary */
#define REAP_NUM 10     /* Number concurrent during reap test */



/*
 * Prototypes
 */
static int open_file(const char *filename);
static int trunc_file(int fd, off_t length);
static char *alloc_buffer(int size);
static int run_write_test();
static int run_suspend_test();
static int run_reap_test();



/*
 * Functions
 */


static int open_file(const char *filename)
{
    int fd;

    if ((filename == NULL) || (*filename == '\0') ||
        strchr(filename, '/'))
    {
        fprintf(stdout, "Test aborted: Invalid filename: %s\n",
                filename ? filename : "(null)");
        return(-EINVAL);
    }

    fprintf(stdout, "Opening file %s... ", filename);
    fflush(stdout);
    fd = open(filename, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd < 0)
    {
        fd = -errno;
        fprintf(stdout, "failed\n\tError opening %s: %s\n",
                filename, strerror(errno));
    }
    else
        fprintf(stdout, "done\n");
    fflush(stdout);

    return(fd);
}


static int trunc_file(int fd, off_t length)
{
    int rc;

    fprintf(stdout, "Truncating to length %ld... ", length);
    fflush(stdout);
    rc = ftruncate(fd, length);
    if (rc < 0)
    {
        rc = -errno;
        fprintf(stdout, "failed\n\tError truncing to %ld: %s\n",
                length, strerror(errno));
    }
    else
        fprintf(stdout, "done\n");
    fflush(stdout);

    return(rc);
}


static char *alloc_buffer(int size)
{
    char *buf;

    fprintf(stdout, "Allocating write buffer... ");
    fflush(stdout);
    buf = (char *)malloc(size * sizeof(char));
    if (buf != NULL)
    {
        memset(buf, 0, size * sizeof(char));
        fprintf(stdout, "done\n");
    }
    else
        fprintf(stdout, "failed\n");
    fflush(stdout);

    return(buf);
}


static int run_write_test()
{
    int fd, rc, count;
    off_t cur;
    char *buffer;

    buffer = alloc_buffer(PAGE_SIZE);
    if (buffer == NULL)
        return(-ENOMEM);

    fd = open_file(WRITE_FILE);
    if (fd < 0)
        return(fd);

    rc = trunc_file(fd, SEEK_SIZE);
    if (rc < 0)
        return(rc);

    fprintf(stdout, "Writing blocks: ");
    fflush(stdout);

    cur = 0;
    while (cur < SEEK_SIZE)
    {
        count = SEEK_SIZE - cur;
        if (count > PAGE_SIZE)
            count = PAGE_SIZE;

        rc = write(fd, buffer, count);
        if (rc < 0)
        {
            if ((errno == EAGAIN) || (errno == EINTR))
                continue;
            rc = -errno;
            fprintf(stdout, "write error: %s\n",
                    strerror(errno));
            break;
        }

        cur += count;

        if ((cur % (PAGE_SIZE * 10)) == 0)
            fprintf(stdout, ".");
        if ((cur % (PAGE_SIZE * 10 * 40)) == 0)
            fprintf(stdout, " %10ld bytes\n\t\t", cur);
        if (rc == 0)
        {
            fprintf(stdout, "file closed prematurely\n");
            break;
        }
        fflush(stdout);
    }

    if ((cur % (PAGE_SIZE * 10 * 40)) != 0)
        fprintf(stdout, " %10ld bytes\n", cur);

    free(buffer);
    close(fd);
    unlink(WRITE_FILE);

    return(rc);
}

static int run_suspend_test()
{
    int fd, rc;
    off_t cur;
    struct aiocb cb;
    struct aiocb *list[1];
    char *buffer;
    struct timeval tv;
    struct timespec ts;

    cur = 0;
    memset(&cb, 0, sizeof(struct aiocb));

    buffer = alloc_buffer(PAGE_SIZE);
    if (buffer == NULL)
        return(-ENOMEM);

    fd = open_file(SUSPEND_FILE);
    if (fd < 0)
        return(fd);

    rc = trunc_file(fd, SEEK_SIZE);
    if (rc < 0)
        return(rc);

    cb.aio_fildes = fd;
    cb.aio_lio_opcode = LIO_WRITE;
    cb.aio_buf = buffer;

    list[0] = &cb;

    fprintf(stdout, "Writing blocks: ");
    fflush(stdout);

    while (cur < SEEK_SIZE)
    {
        cb.aio_offset = cur;
        cb.aio_nbytes = SEEK_SIZE - cb.aio_offset;
        if (cb.aio_nbytes > PAGE_SIZE)
            cb.aio_nbytes = PAGE_SIZE;

        rc = aio_write(&cb);
        if (rc < 0)
        {
            rc = -errno;
            fprintf(stdout, "submit error: %s\n",
                    strerror(errno));
            break;
        }

        ts.tv_sec = 0;
        ts.tv_nsec = 1;
        /* nanosleep(&ts, NULL); */
        gettimeofday(&tv, NULL);
        /* ts.tv_sec = tv.tv_sec + 60; */
        ts.tv_sec = 60;  /* aio is relative timeout */
        ts.tv_nsec = 0;

        /* Cast because GCC warns.  I don't know if GCC is wrong yet */
        rc = aio_suspend((const struct aiocb * const *)list, 1, &ts);
        if (rc < 0)
        {
            rc = -errno;
            fprintf(stdout, "suspend error: %s\n",
                    strerror(errno));
            break;
        }

        rc = aio_error(&cb);
        
        if (rc == EINPROGRESS)
        {
            rc = -EINPROGRESS;
            fprintf(stdout, "EINPROGRESS (how did this happen?)\n");
            break;
        }
        else if (rc != 0)
        {
            rc = -errno;
            fprintf(stdout, "query error: %s\n", 
                    strerror(errno));
            break;
        }
        else
        {
            rc = aio_return(&cb);
            if (rc < 0)
            {
                fprintf(stdout,
                        "return error (how did this happen?)\n");
                break;
            }
            cur += cb.aio_nbytes;

            if ((cur % (PAGE_SIZE * 10)) == 0)
                fprintf(stdout, ".");
            if ((cur % (PAGE_SIZE * 10 * 40)) == 0)
                fprintf(stdout, " %10ld bytes\n\t\t", cur);
            if (rc == 0)
            {
                fprintf(stdout, "file closed prematurely\n");
                break;
            }
        }
        fflush(stdout);
    }

    if ((cur % (PAGE_SIZE * 10 * 40)) != 0)
        fprintf(stdout, " %10ld bytes\n", cur);

    free(buffer);
    close(fd);
    unlink(SUSPEND_FILE);

    return(rc);
}


static int run_reap_test()
{
    int fd, rc, i, num_done;
    off_t cur;
    struct aiocb cb[REAP_NUM];
    struct aiocb *cbp[REAP_NUM];
    struct aiocb *list[REAP_NUM];
    char *buffer;
    struct timeval tv;
    struct timespec ts;

    cur = 0;
    memset(cb, 0, REAP_NUM * sizeof(struct aiocb));

    buffer = alloc_buffer(PAGE_SIZE * REAP_NUM);
    if (buffer == NULL)
        return(-ENOMEM);

    fd = open_file(REAP_FILE);
    if (fd < 0)
        return(fd);

    rc = trunc_file(fd, SEEK_SIZE);
    if (rc < 0)
        return(rc);

    for (i = 0; i < REAP_NUM; i++)
    {
        cb[i].aio_fildes = fd;
        cb[i].aio_lio_opcode = LIO_WRITE;
        cb[i].aio_buf = buffer + (i * PAGE_SIZE);
        cbp[i] = &(cb[i]);
    }

    fprintf(stdout, "Writing blocks: ");
    fflush(stdout);

    while (cur < SEEK_SIZE)
    {
        for (i = 0; i < REAP_NUM; i++)
        {
            cb[i].aio_offset = cur + (i * PAGE_SIZE);
            cb[i].aio_nbytes = SEEK_SIZE - cb[i].aio_offset;
            if (cb[i].aio_nbytes > PAGE_SIZE)
                cb[i].aio_nbytes = PAGE_SIZE;
            else
            {
                i++;
                break;
            }
        }

        fflush(stdout);
        rc = lio_listio(LIO_WRITE, cbp, i, NULL);
        if (rc < 0)
        {
            rc = -errno;
            fprintf(stdout, "listio error: %s\n", strerror(errno));
            break;
        }

        fflush(stdout);
        ts.tv_sec = 0;
        ts.tv_nsec = 1;
        /* nanosleep(&ts, NULL); */
        gettimeofday(&tv, NULL);
        /* ts.tv_sec = tv.tv_sec + 60; */
        ts.tv_sec = 60;
        ts.tv_nsec = 0;

        rc = aio_reap(&ts, i, list, REAP_NUM, &num_done);
        if (rc < 0)
        {
            rc = -errno;
            fprintf(stdout, "reap error: %s\n", strerror(errno));
            break;
        }

        for (i = 0; i < num_done; i++)
        {
            rc = aio_return(cbp[i]);
            if (rc < 0)
            {
                fprintf(stdout, "write error: %s\n", strerror(errno));
                break;
            }
            cur += cb[i].aio_nbytes;

            if ((cur % (PAGE_SIZE * 10)) == 0)
                fprintf(stdout, ".");
            if ((cur % (PAGE_SIZE * 10 * 40)) == 0)
                fprintf(stdout, " %8ld bytes\n\t\t", cur);
            if (rc == 0)
            {
                fprintf(stdout, "file closed prematurely\n");
                break;
            }
        }
        fflush(stdout);
    }

    if ((cur % (PAGE_SIZE * 10 * 40)) != 0)
        fprintf(stdout, " %8ld bytes\n", cur);

    free(buffer);
    close(fd);
    unlink(REAP_FILE);

    return(rc);
}


int main(int argc, char *argv[])
{
    int rc;

    aio_init(128);

    fprintf(stdout, "\nStarting write() test\n\n");
    rc = run_write_test();
    if (rc < 0)
    {
        fprintf(stdout, "write() test failed\n");
        return(rc);
    }
    else
        fprintf(stdout, "write() test complete\n");

    fprintf(stdout, "\nStarting aio_suspend() test\n\n");
    rc = run_suspend_test();
    if (rc < 0)
    {
        fprintf(stdout, "aio_suspend() test failed\n");
        return(rc);
    }
    else
        fprintf(stdout, "aio_suspend() test complete\n");

    fprintf(stdout, "\nStarting aio_reap() test\n\n");
    fprintf(stdout, "Page size is %ld\n", (long)PAGE_SIZE);
    rc = run_reap_test();
    if (rc < 0)
    {
        fprintf(stdout, "aio_reap() test failed\n");
        return(rc);
    }
    else
        fprintf(stdout, "aio_reap() test complete\n");

    return(0);
}
