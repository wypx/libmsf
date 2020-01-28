/*
 * NAME
 *	lio_test.c - Testing lio_listio() and aio_reap()
 *
 * AUTHOR
 * 	Joel Becker <joel.becker@oracle.com>
 *
 * DESCRIPTION
 *      This test program tests asyncronous I/O operations via the
 *      libaio-oracle APIs of lio_listio() and aio_reap().
 *
 * MODIFIED   (YYYY/MM/DD)
 *      2002/08/07 - Joel Becker <joel.becker@oracle.com>
 *              Modified for public release.
 *	2002/01/29 - Joel Becker <joel.becker@oracle.com>
 *		Modified for the skgaio rename.
 *	2002/01/25 - Joel Becker <joel.becker@oracle.com>
 *		Initial header description.
 *
 * Copyright (c) 2002,2003 Oracle Corporation.  All rights reserved.
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
#define __USE_GNU  /* UGLY for O_DIRECT */
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>

/* BLKGETSIZE */
#include <sys/mount.h>
/* RAW_MAJOR */
#include <sys/raw.h>

#include "libaio-oracle.h"


extern char *optarg;
extern int optind, opterr, optopt;


long get_number(char *arg)
{
    char *ptr = NULL;
    long res;

    res = strtol(arg, &ptr, 0);
    if ((ptr == arg) || 
        (res == LONG_MIN) ||
        (res == LONG_MAX) ||
        (res < 1))
        return(-EINVAL);

    switch (*ptr)
    {
        case '\0':
            break;

        case 'g':
        case 'G':
            res *= 1024;
            /* FALL THROUGH */

        case 'm':
        case 'M':
            res *= 1024;
            /* FALL THROUGH */

        case 'k':
        case 'K':
            res *= 1024;
            /* FALL THROUGH */

        case 'b':
        case 'B':
            break;

        default:
            return(-EINVAL);
    }

    return(res);
}


void print_usage(int rc)
{
    FILE *output;

    output = rc ? stderr : stdout;

    fprintf(output,
            "Usage: lio_test [-b <buffersize>] [-s <simultaneous>]\n"
            "                [-c <bytecount>] <input_file> <output_file>\n"
            "       <buffersize> must be a multiple of 256 and defaults to 1024\n"
            "       <simultaneous> is the number of simultaneous <buffersize> requests\n"
            "       <bytecount> is the number of bytes to copy.  It defaults to -1\n"
            "               (read the entire file)\n");
    exit(rc);
}

char *init_mem(long buffersize, long simultaneous)
{
    long tot_size;
    long align_size;
    char *new_buf;

    align_size = getpagesize();

    tot_size = ((simultaneous * buffersize) + align_size) *
               sizeof(char);
    new_buf = (char *)malloc(tot_size);
                             
    if (new_buf == NULL)
    {
        fprintf(stderr, "lio_test: Unable to allocate memory: %s.\n",
                strerror(errno));
        exit(-errno);
    }
    fprintf(stdout,
            "buffersize = %ld, simultaneous = %ld, new_buf size = %ld\n"
            "new_buf location = %p, new_buf end = %p\n",
            buffersize, simultaneous, tot_size, 
            new_buf, new_buf + tot_size);

    /* Align on page boundary */
    new_buf = (char *) (((unsigned long)new_buf & ~(align_size - 1)) +
                        align_size);
    fprintf(stdout,
            "new_buf aligned = %p, new_buf + buffer = %p\n",
            new_buf, new_buf + buffersize);

    return(new_buf);
}

struct aiocb64 **init_aio(char *buf, long buffersize, long simultaneous)
{
    long i, offset;
    struct aiocb64 *aiocb_buf;
    struct aiocb64 **aiocbs;

    aiocbs = (struct aiocb64 **)malloc(simultaneous *
                                       sizeof(struct aiocb64 *));
    if (aiocbs == NULL)
    {
        fprintf(stderr, "lio_test: Unable to allocate memory: %s.\n",
                strerror(errno));
        exit(-errno);
    }
    aiocb_buf = (struct aiocb64 *)malloc(simultaneous *
                                         sizeof(struct aiocb64));
    if (aiocb_buf == NULL)
    {
        fprintf(stderr, "lio_test: Unable to allocate memory: %s.\n",
                strerror(errno));
        exit(-errno);
    }

    memset(aiocb_buf, 0, simultaneous * sizeof(struct aiocb64));
    for (i = 0; i < simultaneous; i++)
    {
        offset = buffersize * i;
        aiocbs[i] = &(aiocb_buf[i]);
        aiocbs[i]->aio_nbytes = buffersize;
        aiocbs[i]->aio_offset = offset;
        aiocbs[i]->aio_buf = buf + offset;
    }

    return(aiocbs);
}

void set_aio_op(struct aiocb64 **aiocbs, long count,
                int fd, int op, long offset, long len)
{
    long i;

    if ((op != LIO_READ) && (op != LIO_WRITE))
    {
        fprintf(stderr, "lio_test: Invalid operation\n");
        exit(-EINVAL);
    }

    for (i = 0; (i < count) && (len > 0); i++)
    {
        aiocbs[i]->aio_fildes = fd;
        aiocbs[i]->aio_lio_opcode = op;
        aiocbs[i]->aio_offset = offset;
        if (len < aiocbs[i]->aio_nbytes)
            aiocbs[i]->aio_nbytes = len;
        offset += aiocbs[i]->aio_nbytes;
        len -= aiocbs[i]->aio_nbytes;
    }
}


void parse_options(int argc, char *argv[], long *buffersize,
                   long *simultaneous, long *count, int *od_flags,
                   int *arg_ind)
{
    int c;

    opterr = 0;
    while ((c = getopt(argc, argv, "Db:s:c:")) != EOF)
    {
        switch (c)
        {
            case 'b':
                *buffersize = get_number(optarg);
                if ((*buffersize < 1) ||
                    ((*buffersize % 256) != 0))
                {
                    fprintf(stderr,
                            "lio_test: Invalid size: \"%s\"\n",
                            optarg);
                    print_usage(-EINVAL);
                }
                break;

            case 's':
                *simultaneous = get_number(optarg);
                if (*simultaneous < 1)
                {
                    fprintf(stderr,
                            "lio_test: Invalid size: \"%s\"\n",
                            optarg);
                    print_usage(-EINVAL);
                }
                if (*simultaneous > MAX_AIO_REAP)
                    *simultaneous = MAX_AIO_REAP;
                break;

            case 'c':
                *count = get_number(optarg);
                if ((*count < 1) && (*count != -1))
                {
                    fprintf(stderr,
                            "lio_test: Invalid count: \"%s\"\n",
                            optarg);
                    print_usage(-EINVAL);
                }
                break;

            case 'D':
                *od_flags |= O_DIRECT;
                break;

            case '?':
                fprintf(stderr,
                        "lio_test: Invalid option: \'-%c\'\n", optopt);
                print_usage(-EINVAL);
                break;

            default:
                abort();
                break;
        }
    }

    *arg_ind = optind;
}


int main(int argc, char *argv[])
{
    int infd, outfd, rc, done, completed, arg_ind;
    long buffersize, simultaneous, count, i, left, send, j;
    off_t size;
    int op, od_flags;
    char *buf;
    struct aiocb64 **aiocbs;
    struct aiocb64 **out_aiocbs;
    struct timeval tv;
    struct timespec to;
    struct stat stat_buf;

    buffersize = 1024;
    simultaneous = 1;
    count = -1;
    od_flags = 0;
    parse_options(argc, argv,
                  &buffersize, &simultaneous, &count, &od_flags,
                  &arg_ind);

    if (arg_ind + 1 >= argc)
        print_usage(1);

    infd = open(argv[arg_ind], O_RDONLY | od_flags);
    if (infd < 0)
    {
        fprintf(stderr, "lio_test: Unable to open file \"%s\": %s.\n",
                argv[arg_ind], strerror(errno));
        return(-errno);
    }

    rc = fstat(infd, &stat_buf);
    if (rc != 0)
    {
        fprintf(stderr,
                "lio_test: Unable to guess size of file \"%s\": %s.\n",
                argv[arg_ind], strerror(errno));
        return(-errno);
    }
    if (S_ISREG(stat_buf.st_mode))
        size = stat_buf.st_size;
    else if (S_ISCHR(stat_buf.st_mode))
    {
        if (major(stat_buf.st_rdev) != RAW_MAJOR)
        {
            fprintf(stderr,
                    "lio_test: File \"%s\" is a character device.\n",
                    argv[arg_ind]);
            return -EINVAL;
        }
        rc = ioctl(infd, BLKGETSIZE, &size);
        if (rc && (errno != ENOTTY))
        {
            fprintf(stderr,
                    "lio_test: Unable to determine size of \"%s\": %s.\n",
                    argv[arg_ind], strerror(errno));
            return -errno;
        }
        size = size << 9; /* Linux BLKGETSIZE is 512b sectors */
    }
    else if (S_ISBLK(stat_buf.st_mode))
    {
        rc = ioctl(infd, BLKGETSIZE, &size);
        if (rc)
        {
            fprintf(stderr,
                    "lio_test: Unable to determine size of \"%s\": %s\n",
                    argv[arg_ind], strerror(errno));
            return -errno;
        }
        size = size << 9; /* Linux BLKGETSIZE is 512b sectors */
    }
    /* needs real handling */
    if ((count == -1) || ((size > 0) && (count > size))) 
        count = size;

    outfd = open(argv[arg_ind + 1],
                 O_WRONLY | O_TRUNC | od_flags, 0644);
    if ((outfd < 0) && (errno == ENOENT))
        outfd = open(argv[arg_ind + 1],
                     O_WRONLY | O_CREAT, 0644);

    if (outfd < 0)
    {
        fprintf(stderr, "lio_test: Unable to open file \"%s\": %s.\n",
                argv[arg_ind + 1], strerror(errno));
        return(-errno);
    }

    buf = init_mem(buffersize, simultaneous);
    aiocbs = init_aio(buf, buffersize, simultaneous);
    out_aiocbs = (struct aiocb64 **)malloc(simultaneous *
                                           sizeof(struct aiocb64 *));
    if (out_aiocbs == NULL)
    {
        fprintf(stderr, "lio_test: Unable to allocate memory: %s\n",
                strerror(errno));
        return(-errno);
    }

    if (!count)
        fprintf(stdout, "lio_test: Count == 0, doing nothing.\n");

    i = 0;
    op = LIO_READ;
    while (i < count)
    {
        left = count - i;
        send = (left + buffersize) / buffersize;
        if (send > simultaneous)
            send = simultaneous;
        set_aio_op(aiocbs, simultaneous,
                   op == LIO_READ ? infd : outfd, op, i, left);
        fprintf(stdout, "lio_test: Submitting %ld %s%s...\n",
                send,
                op == LIO_READ ? "read" : "write",
                send > 1 ? "s" : "");
        rc = lio_listio64(LIO_NOWAIT, aiocbs, send, NULL);
        if (rc < 0)
        {
            fprintf(stderr, "lio_test: lio_listio had problems: %s\n",
                    strerror(errno));
            exit(-errno);
        }
    
        done = 0;
        while (done == 0)
        {
            memset(out_aiocbs, 0,
                   simultaneous * sizeof(struct aiocb64 *));
            timerclear(&tv);
            rc = gettimeofday(&tv, NULL);
            to.tv_sec = tv.tv_sec + 5 + 1;  /* 5 secs, plus any usecs */
            to.tv_nsec = 0;
            rc = aio_reap64(&to, send,
                            out_aiocbs, simultaneous,
                            &completed);
            if (rc < 0)
            {
                if (errno == E2BIG)
                    done = 1;
                else if ((errno != EAGAIN) && (errno != ETIMEDOUT))
                {
                    fprintf(stderr,
                            "lio_test: aio_reap had problems: %s\n",
                            strerror(errno));
                    return(-errno);
                }
            }
            else if (completed >= send)
                done = 1;
    
            fprintf(stdout, "lio_test: reaped: %d\n", completed);

            for (j = 0; j < completed; j++)
            {
                rc = aio_return(out_aiocbs[j]);
                if (rc < 0)
                {
                    fprintf(stderr,
                            "lio_test: Error %s at offset %lld: %s\n",
                            op == LIO_READ ? "reading" : "writing",
                            out_aiocbs[j]->aio_offset,
                            strerror(errno));
                    return(-errno);
                }
                else if (rc < out_aiocbs[j]->aio_nbytes)
                {
                    fprintf(stderr,
                            "lio_test: Only %s %d bytes at offset %lld\n",
                            op == LIO_READ ? "read" : "wrote",
                            rc, out_aiocbs[j]->aio_offset);
                    return(-errno);
                }
                else if (op == LIO_WRITE)
                    i += out_aiocbs[j]->aio_nbytes;
            }
        }

        if (op == LIO_READ)
            op = LIO_WRITE;
        else
            op = LIO_READ;
    }

    close(infd);
    close(outfd);

    /* free(buf); */
    free(out_aiocbs);
    free((struct aiocb64 *)aiocbs[0]);
    free(aiocbs);

    return(0);
}
