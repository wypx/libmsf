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
#include "msf_threadpool.h"


#define THREAD 4
#define SIZE   8192
#define QUEUES 64

/*
 * Warning do not increase THREAD and QUEUES too much on 32-bit
 * platforms: because of each thread (and there will be THREAD *
 * QUEUES of them) will allocate its own stack (8 MB is the default on
 * Linux), you'll quickly run out of virtual space.
 */


 threadpool_t *pool[QUEUES];
 s32 tasks[SIZE], left;
 pthread_mutex_t lock;
 
 s32 error;

 
 void dummy_task(void *arg) {
     s32 *pi = (is32nt *)arg;
     *pi += 1;
 
     if(*pi < QUEUES) {
         assert(threadpool_add(pool[*pi], &dummy_task, arg, 0) == 0);
     } else {
         pthread_mutex_lock(&lock);
         left--;
         pthread_mutex_unlock(&lock);
     }
 }
 
 int main(int argc, char **argv)
 {
     int i, copy = 1;
 
     left = SIZE;
     pthread_mutex_init(&lock, NULL);
 
     for(i = 0; i < QUEUES; i++) {
         pool[i] = threadpool_create(THREAD, SIZE, 0);
         assert(pool[i] != NULL);
     }
 
     usleep(10);
 
     for(i = 0; i < SIZE; i++) {
         tasks[i] = 0;
         assert(threadpool_add(pool[0], &dummy_task, &(tasks[i]), 0) == 0);
     }
 
     while(copy > 0) {
         usleep(10);
         pthread_mutex_lock(&lock);
         copy = left;
         pthread_mutex_unlock(&lock);
     }
 
     for(i = 0; i < QUEUES / 2; i++) {
         assert(threadpool_destroy(pool[i], threadpool_default) == 0);
     }

     for(i = QUEUES / 2; i < QUEUES ; i++) {
         assert(threadpool_destroy(pool[i], threadpool_graceful) == 0);
     }
     pthread_mutex_destroy(&lock);
 
     return 0;
 }

