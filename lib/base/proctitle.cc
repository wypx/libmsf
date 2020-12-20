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
#include "proctitle.h"

#include <sys/prctl.h>
#include <unistd.h>

#include "mem.h"

using namespace MSF;

namespace MSF {

/*
 * A process title on Linux, Solaris, and MacOSX can be changed by
 * copying a new title to a place where the program argument argv[0]
 * points originally to.  However, the argv[0] may be too small to hold
 * the new title.  Fortunately, these OSes place the program argument
 * argv[] strings and the environment environ[] strings contiguously
 * and their space can be used for the long new process title.
 *
 * Solaris "ps" command shows the new title only if it is run in
 * UCB mode: either "/usr/ucb/ps -axwww" or "/usr/bin/ps axwww".
 */

/* example
 * ./a.out start
 * memory layout, supporse ptr point to it
 * ./a.out\0start\0HOSTNAME=vbloger\0TERM=linux\0
 * argc = 2;
 * argv[0] = ptr;
 * argv[1] = ptr + strlen(argv[0])+1;
 * argv[2] = 0;
 * environ[0] = ptr + strlen(argv[0])+1 + strlen(argv[1])+1
 * environ[1] = ptr + strlen(argv[0])+1 + strlen(argv[1])+1 +
 * strlen(environ[0])+1 environ[2] = 0;
 */

extern char **environ;
char *os_argv_last;

static char **old_environ;

int ClearEnv(void) {
#if __GLIBC__
  clearenv();

  return 0;
#else
  static char **tmp;

  if (!(tmp = MSF_NEW(char, sizeof *tmp))) return errno;

  tmp[0] = NULL;
  environ = tmp;

  return 0;
#endif
}

int InitSetProcTitle(int argc, char *argv[]) {
  char *p;
  size_t size = 0;
  uint32_t i;

#if 0
    int error;
    char *tmp;

    old_environ = environ;

    /*
     * #include <errno.h>
     * errno program_invocation_name program_invocation_short_name 
     */
#if __GLIBC__
    if (!(tmp = strdup(program_invocation_name)))
        return;

    program_invocation_name = tmp;

    if (!(tmp = strdup(program_invocation_short_name)))
        return;

    program_invocation_short_name = tmp;
#elif __APPLE__
    if (!(tmp = strdup(getprogname())))
       return;

    setprogname(tmp);
#endif

    if ((error = msf_clearenv()))
        goto error;
#endif

  for (i = 0; environ[i]; i++) {
    size += strlen(environ[i]) + 1;
  }

  p = MSF_NEW(char, size);
  if (NULL == p) {
    goto error;
  }

  os_argv_last = argv[0];

  for (i = 0; argv[i]; i++) {
    if (os_argv_last == argv[i]) {
      os_argv_last = argv[i] + strlen(argv[i]) + 1;
    }
  }

  for (i = 0; environ[i]; i++) {
    if (os_argv_last == environ[i]) {
      size = strlen(environ[i]) + 1;
      os_argv_last = environ[i] + size;

      memcpy(p, (char *)environ[i], size);
      environ[i] = (char *)p;
      p += size;
    }
  }

  os_argv_last--;
  return 0;

error:
  environ = old_environ;
  return -1;
}

void SetProcTitle(int argc, char *argv[], char *title) {
  char *p = NULL;

  p = argv[0];

  memcpy(p, (char *)title, os_argv_last - (char *)p);

  if (os_argv_last - (char *)p) {
    memset(p, 0x00, os_argv_last - (char *)p);
  }
}

}  // namespace MSF
