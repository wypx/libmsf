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
#include <time.h>
#include <sys/time.h>

#include <msf_utils.h>

#define msf_tm_sec            tm_sec
#define msf_tm_min            tm_min
#define msf_tm_hour           tm_hour
#define msf_tm_mday           tm_mday
#define msf_tm_mon            tm_mon
#define msf_tm_year           tm_year
#define msf_tm_wday           tm_wday
#define msf_tm_isdst          tm_isdst

void msf_timezone_update(void);
void msf_localtime(time_t s, struct tm *t);
void msf_libc_localtime(time_t s, struct tm *t);
void msf_libc_gmtime(time_t s, struct tm *t);

#define MSF_TIMEZONE(isdst)   (- (isdst ? timezone + 3600 : timezone) / 60)
#define MSF_GETIMEOFDAY(tp)  (void) gettimeofday(tp, NULL);
#define MSF_MLEEP(ms)        (void) usleep(ms * 1000)
#define MSF_SLEEP(s)          (void) sleep(s)

#ifdef MSF_HAVE_LINUX_TIMER
#define MSF_TIMERADD(tvp, uvp, vvp) timeradd((tvp), (uvp), (vvp))
#define MSF_TIMERSUB(tvp, uvp, vvp) timersub((tvp), (uvp), (vvp))
#define MSF_TIMERCLEAR(tvp) timerclear(tvp)
#define MSF_TIMERISSET(tvp) timerisset(tvp)
#else
#define MSF_TIMERADD(tvp, uvp, vvp)     \
    do {                                \
        (vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;      \
        (vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;   \
        if ((vvp)->tv_usec >= 1000000) {        \
            (vvp)->tv_sec++;                    \
            (vvp)->tv_usec -= 1000000;          \
        }                                       \
    } while (0)

#define MSF_TIMERSUB(tvp, uvp, vvp)                         \
    do {                                                    \
        (vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;      \
        (vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;   \
        if ((vvp)->tv_usec < 0) {                           \
            (vvp)->tv_sec--;                                \
            (vvp)->tv_usec += 1000000;                      \
        }                                                   \
    } while (0)

#define MSF_TIMERCLEAR(tvp)	(tvp)->tv_sec = (tvp)->tv_usec = 0
#define MSF_TIMERISSET(tvp)	((tvp)->tv_sec || (tvp)->tv_usec)

#endif

/** Return true iff the tvp is related to uvp according to the relational
 * operator cmp.  Recognized values for cmp are ==, <=, <, >=, and >. */
#define MSF_TIMERCMP(tvp, uvp, cmp)         \
    (((tvp)->tv_sec == (uvp)->tv_sec) ?     \
     ((tvp)->tv_usec cmp (uvp)->tv_usec) :  \
     ((tvp)->tv_sec cmp (uvp)->tv_sec))


