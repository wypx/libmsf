
#include <butil/logging.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <malloc.h>
#include <paths.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>
#include <termios.h>
#include <unistd.h>

int32_t get_pid() {
#if (defined(WIN32) || defined(_WIN32))
  return static_cast<int32_t>(::GetCurrentProcessId());
#else
  return static_cast<int32_t>(::getpid());
#endif
}

int f_read(const char *path, void *buffer, int max) {
  int f;
  int n;

  if ((f = open(path, O_RDONLY)) < 0) return -1;
  n = read(f, buffer, max);
  close(f);
  return n;
}

int f_read_string(const char *path, char *buffer, int max) {
  if (max <= 0) return -1;
  int n = f_read(path, buffer, max - 1);

  buffer[(n > 0) ? n : 0] = 0;
  return n;
}

char *psname(int pid, char *buffer, int maxlen) {
  char buf[512];
  char path[64];
  char *p;

  if (maxlen <= 0) return NULL;
  *buffer = 0;
  sprintf(path, "/proc/%d/stat", pid);
  if ((f_read_string(path, buf, sizeof(buf)) > 4) &&
      ((p = strrchr(buf, ')')) != NULL)) {
    *p = 0;
    if (((p = strchr(buf, '(')) != NULL) && (atoi(buf) == pid)) {
      strncpy(buffer, p + 1, maxlen);
    }
  }
  return buffer;
}

static int _pidof(const char *name, pid_t **pids) {
  const char *p;
  char *e;
  DIR *dir;
  struct dirent *de;
  pid_t i;
  int count;
  char buf[256];

  count = 0;
  *pids = NULL;
  if ((p = strchr(name, '/')) != NULL) name = p + 1;
  if ((dir = opendir("/proc")) != NULL) {
    while ((de = readdir(dir)) != NULL) {
      i = strtol(de->d_name, &e, 10);
      if (*e != 0) continue;
      if (strcmp(name, psname(i, buf, sizeof(buf))) == 0) {
        if ((*pids = (pid_t *)realloc(*pids, sizeof(pid_t) * (count + 1))) ==
            NULL) {
          return -1;
        }
        (*pids)[count++] = i;
      }
    }
  }
  closedir(dir);
  return count;
}

int pidof(const char *name) {
  pid_t *pids;
  pid_t p;

  if (_pidof(name, &pids) > 0) {
    p = *pids;
    free(pids);
    return p;
  }
  return -1;
}

int killall(const char *name, int sig) {
  pid_t *pids;
  int i;
  int r;

  if ((i = _pidof(name, &pids)) > 0) {
    r = 0;
    do {
      r |= ::kill(pids[--i], sig);
    } while (i > 0);
    free(pids);
    return r;
  }
  return -2;
}

bool IsProcessRunning(long pid) {
#if defined(WIN32)
  HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);
  DWORD ret = WaitForSingleObject(process, 0);
  CloseHandle(process);
  return (ret == WAIT_TIMEOUT);
#else
  return (::kill(pid, 0) == 0);
#endif
}

int kill_pid(int pid) {
  int deadcnt = 20;
  struct stat s;
  char pfile[32];

  if (pid > 0) {
    ::kill(pid, SIGTERM);

    sprintf(pfile, "/proc/%d/stat", pid);
    while (deadcnt--) {
      usleep(100 * 1000);
      if ((stat(pfile, &s) > -1) && (s.st_mode & S_IFREG)) {
        kill(pid, SIGKILL);
      } else
        break;
    }
    return 1;
  }

  return 0;
}

int stop_process(char *name) {
  int deadcounter = 20;

  if (pidof(name) > 0) {
    killall(name, SIGTERM);
    while (pidof(name) > 0 && deadcounter--) {
      usleep(100 * 1000);
    }
    if (pidof(name) > 0) {
      killall(name, SIGKILL);
    }
    return 1;
  }
  return 0;
}
