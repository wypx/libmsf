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
#ifndef BASE_ARCH_H_
#define BASE_ARCH_H_

namespace MSF {

#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || \
    defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || \
    defined(__NT__) || defined(WINCE) || defined(_WIN32_WCE)
#define OS_WINDOWS 1
#define PLATFORM_ID "Windows"

#elif defined(linux) || defined(__linux__) || defined(__linux)
#define OS_LINUX 1
#define PLATFORM_ID "Linux"

#elif defined(__CYGWIN__)
#define OS_CYGWIN 1
#define PLATFORM_ID "Cygwin"

#elif defined(__MINGW32__)
#define OS_MINGW32 1
#define PLATFORM_ID "MinGW"

#elif defined(__APPLE__) || defined(__apple__)
#define OS_APPLE 1
#define PLATFORM_ID "Darwin"

#elif defined(__FreeBSD__) || defined(__FreeBSD)
#define OS_FREEBSD 1
#define PLATFORM_ID "FreeBSD"

#elif defined(__NetBSD__) || defined(__NetBSD)
#define OS_NETBSD 1
#define PLATFORM_ID "NetBSD"

#elif defined(__OpenBSD__) || defined(__OPENBSD)
#define OS_OPENBSD 1
#define PLATFORM_ID "OpenBSD"

#elif defined(__sun) || defined(sun)
#define OS_SUN 1
#define PLATFORM_ID "SunOS"

#elif defined(_AIX) || defined(__AIX) || defined(__AIX__) || defined(__aix) || \
    defined(__aix__)
#define OS_AIX 1
#define PLATFORM_ID "AIX"

#elif defined(__sgi) || defined(__sgi__) || defined(_SGI)
#define OS_IRIX 1
#define PLATFORM_ID "IRIX"

#elif defined(__hpux) || defined(__hpux__)
#define OS_HPUX 1
#define PLATFORM_ID "HP-UX"

#elif defined(__HAIKU__)
#define OS_HAIKU 1
#define PLATFORM_ID "Haiku"

#elif defined(__BeOS) || defined(__BEOS__) || defined(_BEOS)
#define OS_BEOS 1
#define PLATFORM_ID "BeOS"

#elif defined(__QNX__) || defined(__QNXNTO__)
#define OS_QNX 1
#define PLATFORM_ID "QNX"

#elif defined(__tru64) || defined(_tru64) || defined(__TRU64__)
#define OS_TRU64 1
#define PLATFORM_ID "Tru64"

#elif defined(__riscos) || defined(__riscos__)
#define OS_RISCOS 1
#define PLATFORM_ID "RISCos"

#elif defined(__sinix) || defined(__sinix__) || defined(__SINIX__)
#define OS_SINIX 1
#define PLATFORM_ID "SINIX"

#elif defined(__UNIX_SV__)
#define OS_UNIXSV 1
#define PLATFORM_ID "UNIX_SV"

#elif defined(__bsdos__)
#define OS_BSDOS 1
#define PLATFORM_ID "BSDOS"

#elif defined(_MPRAS) || defined(MPRAS)
#define OS_MPRAS 1
#define PLATFORM_ID "MP-RAS"

#elif defined(__osf) || defined(__osf__)
#define OS_OSF1 1
#define PLATFORM_ID "OSF1"

#elif defined(_SCO_SV) || defined(SCO_SV) || defined(sco_sv)
#define OS_SCOSV 1
#define PLATFORM_ID "SCO_SV"

#elif defined(__ultrix) || defined(__ultrix__) || defined(_ULTRIX)
#define OS_ULTRIX 1
#define PLATFORM_ID "ULTRIX"

#elif defined(__XENIX__) || defined(_XENIX) || defined(XENIX)
#define OS_XENIX 1
#define PLATFORM_ID "Xenix"

#elif defined(__WATCOMC__)
#if defined(__LINUX__)
#define OS_LINUX 1
#define PLATFORM_ID "Linux"

#elif defined(__DOS__)
#define OS_DOS 1
#define PLATFORM_ID "DOS"

#elif defined(__OS2__)
#define OS_OS2 1
#define PLATFORM_ID "OS2"

#elif defined(__WINDOWS__)
#define OS_WINDOWS3X 1
#define PLATFORM_ID "Windows3x"

#else /* unknown platform */
#define OS_UNKNOWN 1
#define PLATFORM_ID
#endif

#else /* unknown platform */

#define OS_UNKNOWN 1
#define PLATFORM_ID

#endif

/* For windows compilers MSVC and Intel we can determine
   the architecture of the compiler being used.  This is because
   the compilers do not have flags that can change the architecture,
   but rather depend on which compiler is being used
*/
#if defined(__x86_64__)
#define ARCH_X86_64 1
#define ARCHITECTURE_ID "X86_64"

#elif define(__i386__)
#define ARCH_I386 1
#define ARCHITECTURE_ID "I386"

#elif defined(_M_X64) || defined(_M_AMD64)
#define ARCH_X64 1
#define ARCHITECTURE_ID "x64"

#elif defined(_M_IX86)
#define ARCH_X86 1
#define ARCHITECTURE_ID "X86"

#elif defined(_M_IA64)
#define ARCH_IA64 1
#define ARCHITECTURE_ID "IA64"

#elif defined(_M_ARM)
#if _M_ARM == 4
#define ARCH_ARMV4I 1
#define ARCHITECTURE_ID "ARMV4I"
#elif _M_ARM == 5
#define ARCH_ARMV5I 1
#define ARCHITECTURE_ID "ARMV5I"
#else
#define ARCH_ARMV 1
#define ARCHITECTURE_ID "ARMV" STRINGIFY(_M_ARM)
#endif

#elif defined(__arm__) || defined(__ICCARM__)
#define ARCH_ARM 1
#define ARCHITECTURE_ID "ARM"

#elif defined(__aarch64__) || defined(_M_ARM64)
#define ARCH_ARM64 1
#define ARCHITECTURE_ID "ARM64"

#elif defined(_M_MIPS)
#define ARCH_MIPS 1
#define ARCHITECTURE_ID "MIPS"

#elif defined(__powerpc__) || defined(__ppc__)
#define ARCH_PPC 1
#define ARCHITECTURE_ID "PPC"

#elif defined(_M_SH)
#define ARCH_SHX 1
#define ARCHITECTURE_ID "SHx"

#else /* unknown architecture */
#define ARCH_UNKNOWN 1
#define ARCHITECTURE_ID ""
#endif

struct Architecture {
  bool intel_pclmul_ = false;
  bool intel_sse42_ = false;
  bool intel_sse41_ = false;
  bool intel_ssse3_ = false;
  bool intel_sse3_ = false;
  bool intel_sse2_ = false;
  bool intel_aesni_ = false;

  /* true if we have ARM NEON or ASIMD abilities */
  bool neon_ = false;
  /* true if we have AArch64 CRC32/CRC32C abilities */
  bool aarch64_crc32_ = false;
  /* true if we have AArch64 PMULL abilities */
  bool aarch64_pmull_ = false;

  bool ppc_crc32_ = false;
};

Architecture* GetArchitecture();

}  // namespace MSF

#endif