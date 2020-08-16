/*
 * Copyright (c) 2005 Mellanox Technologies Ltd.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * $Id$
 *
 * Author: Michael S. Tsirkin <mst@mellanox.co.il>
 */

#ifndef GET_CLOCK_H
#define GET_CLOCK_H

#include <stdint.h>

namespace MSF {

#if defined(__x86_64__) || defined(__i386__)
/* Note: only x86 CPUs which have rdtsc instruction are supported. */
static inline uint64_t get_cycles() {
  unsigned low, high;
  unsigned long long val;
  asm volatile("rdtsc" : "=a"(low), "=d"(high));
  val = high;
  val = (val << 32) | low;
  return val;
}
#elif defined(__aarch64__)

static inline uint64_t get_cycles()
{
  uint64_t val;
  asm volatile("isb" : : : "memory");
  asm volatile("mrs %0, cntvct_el0" : "=r" (val));
  return val;
}

#endif

double get_cpu_mhz(int no_cpu_freq_warn = 0);

} // namespace base

#endif
