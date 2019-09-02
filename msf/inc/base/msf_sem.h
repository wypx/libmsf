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
#ifndef __MSF_SEM__
#define __MSF_SEM__

#include <semaphore.h>
#include <msf_utils.h>

#define msf_sem_init(sem)           sem_init(sem, 0, 0)
#define msf_sem_post(sem)           sem_post(sem)
#define msf_sem_destroy(sem)        sem_destroy(sem)
#define msf_sem_getvalue(sem, var)  sem_getvalue(sem, var)
s32 msf_sem_wait(sem_t* psem, s32 mswait);
#endif
