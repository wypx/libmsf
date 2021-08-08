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
#ifndef BASE_AFFINITY_H_
#define BASE_AFFINITY_H_

#include <stdint.h>

namespace MSF {

//! Thread priorities
enum class ThreadPriority : uint8_t {
  IDLE = 0x00,     //!< Idle thread priority
  LOWEST = 0x1F,   //!< Lowest thread priority
  LOW = 0x3F,      //!< Low thread priority
  NORMAL = 0x7F,   //!< Normal thread priority
  HIGH = 0x9F,     //!< High thread priority
  HIGHEST = 0xBF,  //!< Highest thread priority
  REALTIME = 0xFF  //!< Realtime thread priority
};

int process_pin_to_cpu(uint32_t cpu_id);
int thread_pin_to_cpu(uint32_t cpu_id);
int msf_set_nice(int increment);
int msf_get_priority(int which, int who);
int msf_set_priority(int which, int who, int priority);
uint32_t msf_get_current_cpu(void);

}  // namespace MSF

#endif