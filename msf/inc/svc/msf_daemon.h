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
#ifndef __MSF_DAEMON_H__
#define __MSF_DAEMON_H__

#include <msf_list.h>
#include <msf_file.h>
#include <msf_network.h>
#include <msf_process.h>

/* Supper App Service Process: Process Manager*/
/* Micro service framework manager*/
struct msf_daemon {
    u32 msf_state;
    s8  msf_author[32];
    s8  msf_version[32];
    s8  msf_desc[64];
    s8  *msf_conf;
    s8  *msf_name;
    u32 msf_proc_num;
    struct process_desc *msf_proc_desc;
} __attribute__((__packed__));

#endif
