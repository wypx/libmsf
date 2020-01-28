/*
 * NAME
 *	libaio-oracle.h - User header for libaio-oracle
 *
 * AUTHOR
 * 	Joel Becker <joel.becker@oracle.com>
 *
 * DESCRIPTION
 *      This is the header for users of libaio-oracle.  It defines
 *      the interface to asynchronous I/O operations.
 *
 * MODIFIED   (YYYY/MM/DD)
 *      2003/06/19 - Joel Becker <joel.becker@oracle.com>
 *              Added aio_poll() bits.
 *      2002/09/13 - Joel Becker <joel.becker@oracle.com>
 *              Added aio_cancel() bits.
 *      2002/08/07 - Joel Becker <joel.becker@oracle.com>
 *              Modified for public release.
 *      2002/01/29 - Joel Becker <joel.becker@oracle.com>
 *              Renamed to skgaio.h
 *	2002/01/25 - Joel Becker <joel.becker@oracle.com>
 *		Initial header description.
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

#ifndef _SKGAIO_H
#define _SKGAIO_H

#define IOCB_CMD_READ		0
#define IOCB_CMD_WRITE		1
#define IOCB_CMD_NOP		2
#define IOCB_CMD_CANCEL		3
#define IOCB_CMD_FSYNC		4
#define IOCB_CMD_FDSYNC		5
#define IOCB_CMD_RUNNING	6
#define IOCB_CMD_DONE		7

/* Maximum number of events to retrieve at once */
#define MAX_EVENTS 512
#define MAX_AIO_REAP MAX_EVENTS

#include <stdlib.h>
#include <asm/unistd.h>
#include <linux/types.h>
#include <signal.h>
/*
 * we always use a 64bit off_t when communicating
 * with userland.  its up to libraries to do the
 * proper padding and aio_error abstraction
 *
 * FIXME: this must change from glibc's definition
 * as we do *not* use the sigevent structure which
 * is big and bloated.
 */
struct aiocb64 {
  int aio_fildes;               /* File desriptor.  */
  short aio_lio_opcode;           /* Operation to be performed.  */
  short aio_reqprio;              /* Request priority offset.  */
  void *aio_buf;       			/* Location of buffer.  */
  size_t aio_nbytes;            /* Length of transfer.  */
  loff_t aio_offset;		/* File offset.  */
  /* these are internal to the kernel/libc. */
  long __aio_key; // kernel sets this to -1 if completed
	  				// otherwise >= 0 (the request#)
  void * __aio_data;  // pointer to be returned in event's data 
  int __error_code;
};



#ifdef DEBUG
#define dbg_printf(fmt,arg...)\
	printf(fmt, ##arg)
#else
#define dbg_printf(fmt,arg...)\
	do { } while(0);
#endif


#define aiocb 		aiocb64
#define aio_read 	aio_read64
#define aio_write 	aio_write64
#define aio_poll 	aio_poll64
#define aio_error 	aio_error64
#define aio_return 	aio_return64
#define aio_cancel 	aio_cancel64
#define aio_suspend 	aio_suspend64
#define	lio_listio	lio_listio64 
#define aio_reap        aio_reap64


/*  Initialize async i/o with the given maximum number of requests */
int aio_init(int max_requests);

/* Enqueue read request for given number of bytes and the given priority.  */
int aio_read64(struct aiocb64 *aiocbp);

/* Enqueue write request for given number of bytes and the given priority.  */
int aio_write64(struct aiocb64 *aiocbp);

/* Enqueue a poll request for a given fd. */
int aio_poll64(struct aiocb64 *aiocbp);
 
/*
 * Returns the status of the aiocb.
 * If the operation is incomplete, the return value is undefined
 * < -1 is -errno for the call.
 * >= -1 is the return code of the completed operation
 */
ssize_t aio_return64(struct aiocb64 *aiocbp);

/*
 * Returns the error status of the aiocb.
 * < 0 is -errno for the call.
 * 0 is successfully complete
 * EINPROGRESS is not complete at all.
 * > 0 is errno for unsuccessful completion.
 */
int aio_error64(struct aiocb64 *aiocbp);

/*
 * Try to cancel asynchronous I/O requests outstanding against file
 * descriptor FILDES.
 */
int aio_cancel64 ( int fildes,  struct aiocb64 *aiocbp);
 
/*
 * Suspend calling thread until at least one of the asynchronous I/O
 * operations referenced by LIST has completed.
 */
int aio_suspend64(const struct aiocb64 * const list[],int nent,
		  const struct timespec *timeout); 

/*
 * Suspend calling thread until waitfor asynchronouse I/O operations
 * outstanding have completed.
 */
int aio_reap64(struct timespec *timeout, int waitfor,
               struct aiocb *out_list[], int listsize,
               int *completed_count);


int lio_listio64(int mode, struct aiocb64 * const list[], int nent,
                 struct sigevent *__restrict __sig); 

/* Operation codes for `aio_lio_opcode'.  */
enum
{
    LIO_READ,
#define LIO_READ LIO_READ
    LIO_WRITE,
#define LIO_WRITE LIO_WRITE
    LIO_NOP,
#define LIO_NOP LIO_NOP
    LIO_POLL,
#define LIO_POLL LIO_POLL
};
 
/* Return values of cancelation function.  */
enum
{
    AIO_CANCELED,
#define AIO_CANCELED AIO_CANCELED
    AIO_NOTCANCELED,
#define AIO_NOTCANCELED AIO_NOTCANCELED
    AIO_ALLDONE
#define AIO_ALLDONE AIO_ALLDONE
};

 
/* Synchronization options for `lio_listio' function.  */
enum
{
    LIO_WAIT,
#define LIO_WAIT LIO_WAIT
    LIO_NOWAIT
#define LIO_NOWAIT LIO_NOWAIT
};

#endif /* _SKGAIO_H */
