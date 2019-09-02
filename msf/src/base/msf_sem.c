/**************************************************************************
*
* Copyright (c) 2017-2018, luotang.me <wypx520@gmail.com>, China.
* All rights reserved.
*
* Distributed under the terms of the GNU General Public License v2.
*
* This software is provided 'as is' with no explicit or implied warranties
* in respect of its properties, including, but not limited to, correctness
* and/or fitness for purpose.
*
**************************************************************************/
#include <msf_sem.h>

s32 msf_sem_wait(sem_t* psem, s32 mswait)
{

    s32 rv = 0;

    if (unlikely(!psem)) 
        return -1;

    if (MSF_NO_WAIT == mswait) {
        while ((rv = sem_trywait(psem)) != 0
                && errno == EINTR) {
                usleep(1000);
        }
    } else if (MSF_WAIT_FOREVER == mswait) {
        while ((rv = sem_wait(psem)) != 0 
                && errno == EINTR) {
            usleep(1000);
        }
    } else {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts );
        ts.tv_sec += (mswait / 1000 );
        ts.tv_nsec += ( mswait % 1000 ) * 1000;
        while ((rv = sem_timedwait(psem, &ts)) != 0
                && errno == EINTR) {
                usleep(1000);
        }
    } 
    return rv;
}

