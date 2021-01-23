#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sched.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int preserve_ns(const int pid, const char *ns) {
  int ret;
/* 5 /proc + 21 /int_as_str + 3 /ns + 20 /NS_NAME + 1 \0 */
#define __NS_PATH_LEN 50
  char path[__NS_PATH_LEN];

  /* This way we can use this function to also check whether namespaces
   * are supported by the kernel by passing in the NULL or the empty
   * string.
   */
  ret = snprintf(path, __NS_PATH_LEN, "/proc/%d/ns%s%s", pid,
                 !ns || strcmp(ns, "") == 0 ? "" : "/",
                 !ns || strcmp(ns, "") == 0 ? "" : ns);
  if (ret < 0 || (size_t)ret >= __NS_PATH_LEN) {
    errno = EFBIG;
    return -1;
  }

  return open(path, O_RDONLY | O_CLOEXEC);
}

/**
 * in_same_namespace - Check whether two processes are in the same namespace.
 * @pid1 - PID of the first process.
 * @pid2 - PID of the second process.
 * @ns   - Name of the namespace to check. Must correspond to one of the names
 *         for the namespaces as shown in /proc/<pid/ns/
 *
 * If the two processes are not in the same namespace returns an fd to the
 * namespace of the second process identified by @pid2. If the two processes are
 * in the same namespace returns -EINVAL, -1 if an error occurred.
 */
static int in_same_namespace(pid_t pid1, pid_t pid2, const char *ns) {
  int ns_fd1 = -1, ns_fd2 = -1;
  int ret = -1;
  struct stat ns_st1, ns_st2;

  ns_fd1 = preserve_ns(pid1, ns);
  if (ns_fd1 < 0) {
    /* The kernel does not support this namespace. This is not an
     * error.
     */
    if (errno == ENOENT) return -EINVAL;

    return -1;
  }

  ns_fd2 = preserve_ns(pid2, ns);
  if (ns_fd2 < 0) return -1;

  ret = fstat(ns_fd1, &ns_st1);
  if (ret < 0) return -1;

  ret = fstat(ns_fd2, &ns_st2);
  if (ret < 0) return -1;

  /* processes are in the same namespace */
  if ((ns_st1.st_dev == ns_st2.st_dev) && (ns_st1.st_ino == ns_st2.st_ino))
    return -EINVAL;

  /* processes are in different namespaces */
  return (ns_fd2);
}

bool is_shared_pidns(pid_t pid) {
  int fd = -EBADF;

  if (pid != 1) return false;

  fd = in_same_namespace(pid, getpid(), "pid");
  if (fd == EINVAL) return true;

  return false;
}