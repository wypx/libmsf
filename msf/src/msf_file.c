
#include <msf_file.h>


s32 msf_open_tempfile(u8 *name, u32 persistent, u32 access)
{
    s32  fd;

    fd = open((const s8 *) name, O_CREAT|O_EXCL|O_RDWR,
              access ? access : 0600);

    if (fd != -1 && !persistent) {
		msf_delete_file(name);
    }

    return fd;
}


ssize_t msf_read_file(s32 fd, u8 *buf, size_t size, off_t offset)
{
    ssize_t  n;

#if (MSF_HAVE_PREAD)

    n = pread(fd, buf, size, offset);

    if (n == -1) {
        return -1;
    }

#else

    n = read(fd, buf, size);

    if (n == -1) {
        return -1;
    }


#endif

    return n;
}


ssize_t msf_write_file(s32 fd, u8 *buf, size_t size, off_t offset)
{
    ssize_t  n, written;
	
    written = 0;

#if (MSF_HAVE_PWRITE)

    for ( ;; ) {
        /* pwrite() 把缓存区 buf 开头的 count 个字节写入文件描述符 
         * fd offset 偏移位置上,文件偏移没有改变*/
        n = pwrite(fd, buf + written, size, offset);

        if (n == -1) {
            return -1;
        }

        if ((size_t) n == size) {
            return written;
        }

        offset += n;
        size -= n;
    }

#else


    for ( ;; ) {
        n = write(fd, buf + written, size);

        if (n == -1) {

            return -1;
        }
    }
#endif
}



s32 msf_set_file_time(u8 *name, s32 fd, time_t s) {

	struct timeval  tv[2];

    //tv[0].tv_sec = ngx_time();
    tv[0].tv_usec = 0;
    tv[1].tv_sec = s;
    tv[1].tv_usec = 0;

    if (utimes((s8 *) name, tv) != -1) {
        return -1;
    }

    return -1;

}


s32 msf_create_file_mapping(struct msf_file_mapping *fm) {

	fm->fd = msf_open_file(fm->name, MSF_FILE_RDWR, MSF_FILE_TRUNCATE,
						  MSF_FILE_DEFAULT_ACCESS);

	if (ftruncate(fm->fd, fm->size) == -1) {

 	}

    fm->addr = mmap(NULL, fm->size, PROT_READ|PROT_WRITE, MAP_SHARED,
                    fm->fd, 0);
    if (fm->addr != MAP_FAILED) {
        return 0;
    }

error:
	msf_close_file(fm->fd);
	return -1;
}


void msf_close_file_mapping(struct msf_file_mapping *fm) {

	if (munmap(fm->addr, fm->size) == -1) {

	}

	if (msf_close_file(fm->fd) == -1) {
	}

}



s32 msf_open_glob(struct msf_glob *gl) {
    int  n;

    n = glob((char *) gl->pattern, 0, NULL, &gl->pglob);

    if (n == 0) {
        return 0;
    }

#ifdef GLOB_NOMATCH

    if (n == GLOB_NOMATCH && gl->test) {
        return 0;
    }

#endif

    return -1;
}

s32 msf_read_glob(struct msf_glob *gl, s8 *name) {
    size_t  count;

#ifdef GLOB_NOMATCH
    count = (size_t) gl->pglob.gl_pathc;
#else
    count = (size_t) gl->pglob.gl_matchc;
#endif

    if (gl->n < count) {

        s32 len = (size_t) strlen(gl->pglob.gl_pathv[gl->n]);
        u8 *data = (u8 *) gl->pglob.gl_pathv[gl->n];
        gl->n++;

        return 0;
    }

    return -1;
}


void msf_close_glob(struct msf_glob *gl) {
    globfree(&gl->pglob);
}

/* 设置一把读锁,不等待 */
s32 msf_trylock_rfd(s32 fd, short start, short whence, short len) {

	struct flock lock;

	lock.l_type = F_RDLCK;
	lock.l_start = start;
	lock.l_whence = whence;//SEEK_CUR,SEEK_SET,SEEK_END
	lock.l_len = len;
	lock.l_pid = getpid();
	/* 阻塞方式加锁 */
	if(fcntl(fd, F_SETLK, &lock) == 0)
		return 1;

	return 0;
}

s32 msf_lock_rfd(s32 fd, short start, short whence, short len) {

	struct flock lock;

	lock.l_type = F_RDLCK;
	lock.l_start = start;
	lock.l_whence = whence;//SEEK_CUR,SEEK_SET,SEEK_END
	lock.l_len = len;
	lock.l_pid = getpid();
	/* 阻塞方式加锁 */
	if(fcntl(fd, F_SETLKW, &lock) == 0)
		return 1;

	return 0;
}


s32 msf_trylock_wfd(s32 fd) {

	struct flock  fl;

	/* 这个文件锁并不用于锁文件中的内容,填充为0 */
	msf_memzero(&fl, sizeof(struct flock));
	fl.l_type = F_WRLCK; /*F_WRLCK意味着不会导致进程睡眠*/
	fl.l_whence = SEEK_SET;

	/* 获取互斥锁成功时会返回0,否则返回的其实是errno错误码,
	 * 而这个错误码为EAGAIN或者EACCESS时表示当前没有拿到互斥锁,
	 * 否则可以认为fcntl执行错误*/
	if (fcntl(fd, F_SETLK, &fl) == -1) {
	    return errno;
	}

	return 0;
}

/*
 * 该将会阻塞进程的执行,使用时需要非常谨慎,
 * 它可能会导致worker进程宁可睡眠也不处理其他正常请*/
s32 msf_lock_wfd(s32 fd) {

	struct flock  fl;

	msf_memzero(&fl, sizeof(struct flock));
	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;

	/* 如果返回-1, 则表示fcntl执行错误
	 * 一旦返回0, 表示成功地拿到了锁*/
	if (fcntl(fd, F_SETLKW, &fl) == -1) {
		return errno;
	}

	return 0;
}

s32 msf_unlock_fd(s32 fd) {

	struct flock  fl;

	msf_memzero(&fl, sizeof(struct flock));
	fl.l_type = F_UNLCK;
	fl.l_whence = SEEK_SET;

	if (fcntl(fd, F_SETLK, &fl) == -1) {
		return	errno;
	}

	return 0;
}


#if (MSF_HAVE_POSIX_FADVISE) && !(MSF_HAVE_F_READAHEAD)

s32 ngx_read_ahead(s32 fd, size_t n) {

	s32  err;

    err = posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
    if (err == 0) {
        return 0;
    }

    errno = err;
    return -1;
}

#endif

#if (MSF_HAVE_O_DIRECT)

/* direct AIO可以参考http://blog.csdn.net/bengda/article/details/21871413
 * 普通缓存I/O优点:
 * 缓存 I/O 使用了操作系统内核缓冲区,在一定程度上分离了应用程序空间和实际的物理设备.
 * 缓存 I/O 可以减少读盘的次数, 从而提高性能.
 * 普通缓存I/O优点缺点:
 * 在缓存 I/O 机制中, DMA 方式可以将数据直接从磁盘读到页缓存中,
 * 或者将数据从页缓存直接写回到磁盘上,而不能直接在应用程序地址空间和磁盘之间进行数据传输,
 * 这样的话,数据在传输过程中需要在应用程序地址空间和页缓存之间进行多次数据拷贝操作,
 * 这些数据拷贝操作所带来的 CPU 以及内存开销是非常大的.
 *
 * direct I/O优点:
 * 直接 I/O 最主要的优点就是通过减少操作系统内核缓冲区和应用程序地址空间的数据拷贝次数,
 * 降低了对文件读取和写入时所带来的 CPU 的使用以及内存带宽的占用.
 * 这对于某些特殊的应用程序，比如自缓存应用程序来说，不失为一种好的选择.
 * 如果要传输的数据量很大,使用直接 I/O 的方式进行数据传输,
 * 而不需要操作系统内核地址空间拷贝数据操作的参与,这将会大大提高性能.
 * direct I/O缺点:
 * 设置直接 I/O 的开销非常大,而直接 I/O 又不能提供缓存 I/O 的优势.
 * 缓存 I/O 的读操作可以从高速缓冲存储器中获取数据,而直接I/O 的读数据操作会造成磁盘的同步读,
 * 这会带来性能上的差异, 并且导致进程需要较长的时间才能执行完;
 */
s32 ngx_directio_on(s32 fd) {

	s32  flags;

	flags = fcntl(fd, F_GETFL);

	if (flags == -1) {
		return -1;
	}

	 return fcntl(fd, F_SETFL, flags | O_DIRECT);
}

s32 ngx_directio_off(s32 fd) {

	s32  flags;
	
	flags = fcntl(fd, F_GETFL);

	if (flags == -1) {
		return -1;
	}

	return fcntl(fd, F_SETFL, flags & ~O_DIRECT);
}

#endif


#if (MSF_HAVE_STATFS)

size_t msf_fs_bsize(u8 *name) {

	struct statfs  fs;

	if (statfs((char *) name, &fs) == -1) {
		return 512;
	}

	if ((fs.f_bsize % 512) != 0) {
		return 512;
	}

	return (size_t) fs.f_bsize; /*每个block里面包含的字节数*/
}

#elif (MSF_HAVE_STATVFS)

size_t msf_fs_bsize(u8 *name) {

	struct statvfs  fs;

    if (statvfs((char *) name, &fs) == -1) {
        return 512;
    }

    if ((fs.f_frsize % 512) != 0) {
        return 512;
    }

    return (size_t) fs.f_frsize;
}

#else

size_t msf_fs_bsize(u8 *name) {
    return 512;
}

#endif


s32 msf_create_full_path(u8 *dir, u32 access)
{
    u8    *p, ch;
    s32   err;

    err = 0;

#if (MSF_WIN32)
    p = dir + 3;
#else
    p = dir + 1;
#endif

    for ( /* void */ ; *p; p++) {
        ch = *p;

        if (ch != '/') {
            continue;
        }

        *p = '\0';

        if (msf_create_dir(dir, access) == -1) {
            err = errno;

            switch (err) {
            case EEXIST:
                err = 0;
            case EACCES:
                break;

            default:
                return err;
            }
        }

        *p = '/';
    }

    return err;
}

s32 msf_create_paths(s8 *path, uid_t user) {

	msf_create_dir(path, 0700);

	struct stat  fi;

    if (msf_file_info((const s8 *) path, &fi) == -1) {
        return -1;
    }

	if (fi.st_uid != user) {
        if (chown((const char *) path, user, -1) == -1) {
            return -1;
        }
    }

    if ((fi.st_mode & (S_IRUSR|S_IWUSR|S_IXUSR)) != (S_IRUSR|S_IWUSR|S_IXUSR)) {
        fi.st_mode |= (S_IRUSR|S_IWUSR|S_IXUSR);

        if (chmod((const s8 *) path, fi.st_mode) == -1) {
            return -1;
        }
    }
	return 0;
}


s32 msf_create_pidfile(const s8 *name) {

	u8 pid[MSF_INT64_LEN + 2];
	s32 fd = msf_open_file(name, MSF_FILE_RDWR,
                            MSF_FILE_TRUNCATE, MSF_FILE_DEFAULT_ACCESS);

	return 0;
}

void msf_delete_pidfile(const s8 *name) {
	msf_delete_file(name);
}

pid_t msf_read_pidfile(void) {

	FILE *pid_fp = NULL;
    const s8 *f = "";
    pid_t pid = -1;
    s32 i;

    if (f == NULL) {
        fprintf(stderr, "error: no pid file name defined\n");
        exit(1);
    }

    pid_fp = fopen(f, "r");
    if (pid_fp != NULL) {
        pid = 0;
        if (fscanf(pid_fp, "%d", &i) == 1)
            pid = (pid_t) i;
        fclose(pid_fp);
    } else {
        if (errno != ENOENT) {
            fprintf(stderr, "error: could not read pid file\n");
            fprintf(stderr, "\t%s: %s\n", f, strerror(errno));
            exit(1);
        }
    }
    return pid;
}


s32 msf_check_runningpid(void) {
    pid_t pid;
    pid = msf_read_pidfile();
    if (pid < 2)
        return 0;
    if (kill(pid, 0) < 0)
        return 0;
    fprintf(stderr, "nginx_master is already running!  process id %ld\n", (long int) pid);
    return 1;
}

void msf_write_pidfile(void)
{
    FILE *fp;
    const char *f = NULL;
    fp = fopen(f, "w+");
    if (!fp) {
        fprintf(stderr, "could not write pid file '%s': %s\n", f, strerror(errno));
        return;
    }
    fprintf(fp, "%d\n", (int) getpid());
    fclose(fp);
}


void msf_enable_coredump(void) {

	#if HAVE_PRCTL && defined(PR_SET_DUMPABLE)
	
    /* Set Linux DUMPABLE flag */
    if (prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) != 0) {
        fprintf(stderr, "prctl: %s\n", strerror(errno));
	}

	#endif

    /* Make sure coredumps are not limited */
    struct rlimit rlim;

    if (getrlimit(RLIMIT_CORE, &rlim) == 0) {
        rlim.rlim_cur = rlim.rlim_max;
        if (setrlimit(RLIMIT_CORE, &rlim) == 0) {
            fprintf(stderr, "Enable Core Dumps OK!\n");
            return;
        }
    }
    fprintf(stderr, "Enable Core Dump failed: %s\n",strerror(errno));
}


