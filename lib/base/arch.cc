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
#include "arch.h"

#include <stdio.h>
#include <mutex>

#if defined(ARCH_X86_64)
#include <cpuid.h>
#endif

#if defined(OS_LINUX)
#include <elf.h>
#include <link.h>  // ElfW macro
#include <sys/auxv.h>

#if defined(ARCH_ARM) || defined(ARCH_ARM64)
#include <asm/hwcap.h>
#endif
#endif

#if defined(ARCH_PPC)
#include <sys/auxv.h>
#include <asm/cputable.h>
#endif

using namespace MSF;
namespace MSF {

static void ProbeArchitectureIntel(Architecture* arch) {

#if defined(ARCH_X86_64) || defined(ARCH_I386)

/* http://en.wikipedia.org/wiki/CPUID#EAX.3D1:_Processor_Info_and_Feature_Bits
 */

#define CPUID_PCLMUL (1 << 1)
#define CPUID_SSE42 (1 << 20)
#define CPUID_SSE41 (1 << 19)
#define CPUID_SSSE3 (1 << 9)
#define CPUID_SSE3 (1)
#define CPUID_SSE2 (1 << 26)
#define CPUID_AESNI (1 << 25)

  /* i know how to check this on x86_64... */
  uint32_t eax, ebx, ecx = 0, edx = 0;
  if (!::__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
    return;
  }
  if ((ecx & CPUID_PCLMUL) != 0) {
    arch->intel_pclmul_ = true;
  }
  if ((ecx & CPUID_SSE42) != 0) {
    arch->intel_sse42_ = true;
  }
  if ((ecx & CPUID_SSE41) != 0) {
    arch->intel_sse41_ = true;
  }
  if ((ecx & CPUID_SSSE3) != 0) {
    arch->intel_ssse3_ = true;
  }
  if ((ecx & CPUID_SSE3) != 0) {
    arch->intel_sse3_ = true;
  }
  if ((edx & CPUID_SSE2) != 0) {
    arch->intel_sse2_ = true;
  }
  if ((ecx & CPUID_AESNI) != 0) {
    arch->intel_aesni_ = true;
  }
#endif
}

static void ProbeArchitectureARM(Architecture* arch) {

#if defined(OS_LINUX)
  unsigned long hwcap = ::getauxval(AT_HWCAP);
#if defined(ARCH_ARM)
  arch->neon_ = ((hwcap & HWCAP_NEON) == HWCAP_NEON);
#elif defined(ARCH_ARM64)
  arch->neon_ = ((hwcap & HWCAP_ASIMD) == HWCAP_ASIMD);
  arch->aarch64_crc32_ = ((hwcap & HWCAP_CRC32) == HWCAP_CRC32);
  arch->aarch64_pmull_ = ((hwcap & HWCAP_PMULL) == HWCAP_PMULL);
#endif
  (void)hwcap;
#endif
}

static void ProbeArchitecturePPC(Architecture* arch) {

#if defined(ARCH_PPC)

#ifndef PPC_FEATURE2_VEC_CRYPTO
#define PPC_FEATURE2_VEC_CRYPTO 0x02000000
#endif

#ifndef AT_HWCAP2
#define AT_HWCAP2 26
#endif

  if (::getauxval(AT_HWCAP2) & PPC_FEATURE2_VEC_CRYPTO) {
    arch->ppc_crc32_ = true;
  }
#endif /* HAVE_PPC64LE */
}

static void ProbeArchitecture(Architecture* arch) {
  ProbeArchitectureIntel(arch);
  ProbeArchitectureARM(arch);
  ProbeArchitecturePPC(arch);
}

Architecture* GetArchitecture() {
  static Architecture* global_arch = nullptr;
  if (!global_arch) {
    static std::once_flag once_flag;
    std::call_once(once_flag, [&]() {
      global_arch = new Architecture();
      ProbeArchitecture(global_arch);
    });
  }
  return global_arch;
}

}  // namespace MSF