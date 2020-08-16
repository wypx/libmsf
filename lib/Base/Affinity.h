/**************************************************************************
 *
 * Copyright (c) 2017-2021, luotang.me <wypx520@gmail.com>, China.
 * All rights reserved.
 *
 * Distributed under the terms of the GNU General Public License v2.
 *
 * This software is provided 'as is' with no explicit or implied warranties
 * in respect of its properties, including, but not limited to, correctness
 * and/or fitness for purpose.
 *
 **************************************************************************/
#ifndef LIB_AFFINITY_H_
#define LIB_AFFINITY_H_

#include <stdint.h>

namespace MSF {

int process_pin_to_cpu(uint32_t cpu_id);
int thread_pin_to_cpu(uint32_t cpu_id);
int msf_set_nice(int increment);
int msf_get_priority(int which, int who);
int msf_set_priority(int which, int who, int priority);
uint32_t msf_get_current_cpu(void);

}  // namespace MSF

#endif