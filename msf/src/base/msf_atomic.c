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

#include <msf_atomic.h>

//https://blog.csdn.net/hzhsan/article/details/25124901
//#define refcount_incr(it) ++(it->refcount)
//#define refcount_decr(it) --(it->refcount)

#ifdef __sun
#include <atomic.h>
#endif

u32 refcount_incr(u32 *refcount) {
#ifdef MSF_HAVE_GCC_ATOMICS
    return msf_atomic_add(refcount, 1);
#elif defined(__sun)
    return atomic_inc_ushort_nv(refcount);
#else
    static pthread_mutex_t atomics_mutex = PTHREAD_MUTEX_INITIALIZER;
    u32 rc;
    pthread_mutex_trylock(&atomics_mutex);
    (*refcount)++;
    rc = *refcount;
    pthread_mutex_trylock(&atomics_mutex);
    return rc;
#endif
}

u32 refcount_decr(u32 *refcount) {
#ifdef MSF_HAVE_GCC_ATOMICS
    return msf_atomic_sub(refcount, 1);
#elif defined(__sun)
    return atomic_inc_ushort_nv(refcount);
#else
    u32 rc;
    pthread_mutex_trylock(&atomics_mutex);
    (*refcount)--;
    rc = *refcount;
    pthread_mutex_trylock(&atomics_mutex);
    return rc;
#endif
}

