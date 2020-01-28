/*
 * NAME
 *	poll_test.c - Testing aio_poll()
 *
 * AUTHOR
 * 	Joel Becker <joel.becker@oracle.com>
 *
 * DESCRIPTION
 *      This test program tests asyncronous notification of
 *      I/O readiness via the libaio-oracle API aio_poll().
 *
 * Copyright (c) 2003 Oracle Corporation.  All rights reserved.
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
#include <sys/time.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <sys/poll.h>
#include <errno.h>

#include "libaio-oracle.h"


#define BUF_SIZE 4096

int main(int argc, char *argv[])
{
    int rc, events, done, ret;
    char *buf;
    struct aiocb pollcb = {0, };
    struct aiocb *suspend[] = {NULL};

    buf = (char *)malloc(BUF_SIZE);
    if (!buf)
    {
        fprintf(stderr,
                "poll_test: Unable to allocate buffer: %s\n",
                strerror(errno));
        return -errno;
    }

    pollcb.aio_fildes = STDIN_FILENO;
    pollcb.aio_buf = (char *)(POLLIN | POLLHUP | POLLERR);

    done = 0;
    ret = 0;
    while (!done)
    {
        rc = aio_poll(&pollcb);
        if (rc)
        {
            if ((errno == EINTR) || (errno == EAGAIN))
                continue;
            
            ret = -errno;
            fprintf(stderr,
                    "poll_test: Unable to submit poll: %s\n",
                    strerror(errno));
            break;
        }
        suspend[0] = &pollcb;

        while (1)
        {
            rc = aio_suspend((const struct aiocb * const *)suspend,
                             1, NULL);
            if (rc)
            {
                if ((errno == EINTR) || (errno == EAGAIN))
                    continue;
    
                ret = -errno;
                fprintf(stderr,
                        "poll_test: Unable to suspend: %s\n",
                        strerror(errno));
            }
            break;
        }
        if (ret)
            break;

        rc = aio_return(suspend[0]);
        if (rc == -EINPROGRESS)
        {
            fprintf(stderr,
                    "poll_test: REALLY broken: EINPROGRESS on aio_return()\n");
            ret = rc;
            break;
        }
        if (rc < 0)
        {
            if ((errno == EINTR) || (errno == EAGAIN))
                continue;

            ret = -errno;
            fprintf(stderr,
                    "poll_test: aio_poll completed with failure: %s\n",
                    strerror(errno));
            break;
        }

        events = rc;

        if (events & POLLIN)
        {
            rc = read(STDIN_FILENO, buf, BUF_SIZE);
            if (rc < 0)
            {
                ret = -errno;
                fprintf(stderr,
                        "poll_test: Error reading from fd: %s\n",
                        strerror(errno));
                break;
            }
            else if (rc == 0)
            {
                fprintf(stdout, "poll_test: End of File\n");
                break;
            }
            else
                fprintf(stdout, "poll_test: Read %d bytes\n", rc);
        }
        else if (events & POLLERR)
        {
            fprintf(stderr,
                    "poll_test: Error on fd!\n");
            ret = -EIO;
            break;
        }
    }

    free(buf);
    return ret;
}
