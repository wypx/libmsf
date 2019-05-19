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

#include <msf_utils.h>

#ifndef __MSF_LOG_H__
#define __MSF_LOG_H__
// colors
#define NONE                 "\e[0m"
#define BLACK                "\e[0;30m"
#define L_BLACK              "\e[1;30m"
#define RED                  "\e[0;31m"
#define L_RED                "\e[1;31m"
#define GREEN                "\e[0;32m"
#define L_GREEN              "\e[1;32m"
#define BROWN                "\e[0;33m"
#define YELLOW               "\e[1;33m"
#define BLUE                 "\e[0;34m"
#define L_BLUE               "\e[1;34m"
#define PURPLE               "\e[0;35m"
#define L_PURPLE             "\e[1;35m"
#define CYAN                 "\e[0;36m"
#define L_CYAN               "\e[1;36m"
#define GRAY                 "\e[0;37m"
#define WHITE                "\e[1;37m"

#define BOLD                 "\e[1m"
#define UNDERLINE            "\e[4m"
#define BLINK                "\e[5m"
#define REVERSE              "\e[7m"
#define HIDE                 "\e[8m"
#define CLEAR                "\e[2J"
#define CLRLINE              "\r\e[K" /* or "\e[1K\r" */


enum LOGCOLOR {
    LC_DEBUG = 0,
    LC_VERBOSE,
    LC_INFO,
    LC_WARN,
    LC_ERR,
    LC_ALERT,
    LC_NOTICE,
    LC_CRIT,
    LC_CHAIN,
    LC_MAX,
};

enum loglevel {
    DBG_FATAL = 0,
    DBG_ERROR,
    DBG_WARN,
    DBG_DEBUG,
    DBG_INFO,
    DBG_MAX
};

enum logstat { 
   L_CLOSED,
   L_OPENING,
   L_OPEN,
   L_ERROR,
   L_ZIPING,
};

enum log_type_t {
    LOG_STDIN   = 0, /*stdin*/
    LOG_STDOUT  = 1, /*stdout*/
    LOG_STDERR  = 2, /*stderr*/
    LOG_FILE    = 3,
    LOG_RSYSLOG = 4,
    LOG_MAX_OUTPUT = 255
} ;

s32 msf_log_write(s32 level, s8 *mod, const s8 *func, const s8 *file, s32 line, s8 *fmt, ...);
s32 msf_log_init(const s8 *log_path);
void msf_log_free(void);
#endif
