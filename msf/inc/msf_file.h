
#include <msf_utils.h>

#include <utime.h> /* utime */
#include <sys/time.h> /* utimes */

#include <glob.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/vfs.h>
#include <sys/statfs.h>
#include <sys/prctl.h>
#include <sys/resource.h>

struct msf_file_mapping {
    u8          *name;
    size_t      size;
    void        *addr;
    s32         fd;
}__attribute__((__packed__));


struct msf_dir {
	DIR             *dir;
	struct dirent   *de;
	struct stat     info;

	u32             type:8;
	u32             valid_info:1;
} __attribute__((__packed__));


struct msf_glob {
    size_t  n;
    glob_t  pglob;
    u8      *pattern;
    u32     test;
} __attribute__((__packed__));

struct msf_file {
    s32     fd;
    ino_t   uniq;//文件inode节点号 同一个设备中的每个文件，这个值都是不同的
    time_t  mtime; //文件最后被修改的时间
    off_t   size;
    off_t   fs_size;
	off_t   directio; //生效见ngx_open_and_stat_file  if (of->directio <= ngx_file_size(&fi)) { ngx_directio_on }
    size_t  read_ahead;  /* read_ahead配置，默认0 */

	u32     is_dir:1;
    u32     is_file:1;
    u32     is_link:1;
    u32     is_exec:1;
	u32     is_directio:1; //当文件大小大于directio xxx
} __attribute__((__packed__));



#define msf_stdout               STDOUT_FILENO
#define msf_stderr               STDERR_FILENO
#define msf_set_stderr(fd)       dup2(fd, STDERR_FILENO) 


/*
 * -rw-rw-r--
 * 一共有10位数
 *　　其中： 最前面那个 - 代表的是类型
 *　　中间那三个 rw- 代表的是所有者（user）
 *　　然后那三个 rw- 代表的是组群（group）
 *　　最后那三个 r-- 代表的是其他人（other）*/

#define MSF_FILE_DEFAULT_ACCESS 0644
#define MSF_FILE_OWNER_ACCESS 	0600

#define MSF_FILE_RDONLY          O_RDONLY
#define MSF_FILE_WRONLY          O_WRONLY
#define MSF_FILE_RDWR            O_RDWR
#define MSF_FILE_CREATE_OR_OPEN  O_CREAT
#define MSF_FILE_OPEN            0
#define MSF_FILE_TRUNCATE        (O_CREAT|O_TRUNC)
#define MSF_FILE_APPEND          (O_WRONLY|O_APPEND)
#define MSF_FILE_NONBLOCK        O_NONBLOCK

#define msf_open_file(name, mode, create, access)                            \
    open((const char *) name, mode|create, access)


#define msf_close_file           close

/* unlink 只能删除文件,不能删除目录 
 * unlink函数使文件引用数减一,当引用数为零时,操作系统就删除文件.
 * 但若有进程已经打开文件,则只有最后一个引用该文件的文件描述符关闭,
 * 该文件才会被删除 */
#define msf_delete_file(name)    unlink((const s8 *) name)

s32 msf_open_tempfile(u8 *name, u32 persistent, u32 access);
ssize_t msf_read_file(s32 fd, u8 *buf, size_t size, off_t offset);
ssize_t msf_write_file(s32 fd, u8 *buf, size_t size, off_t offset);


/* rename函数功能是给一个文件重命名,用该函数可以实现文件移动功能
 * 把一个文件的完整路径的盘符改一下就实现了这个文件的移动 
 * rename和mv命令差不多,只是rename支持批量修改
 * mv也能用于改名,但不能实现批量处理(改名时,不支持*等符号的)而rename可以
 * rename后文件的fd和stat信息不变使用 RENAME,原子性地对临时文件进行改名
 * 覆盖原来的 RDB 文件*/

#define msf_rename_file(o, n)    rename((const s8 *) o, (const s8 *) n)

#define msf_change_file_access(n, a) chmod((const s8 *) n, a)

/*  stat系统调用系列包括了fstat,stat和lstat


*/
#define msf_file_info(file, sb)  stat((const s8 *) file, sb)

#define msf_fd_info(fd, sb)      fstat(fd, sb)


#define msf_link_info(file, sb)  lstat((const s8 *) file, sb)


#define msf_is_dir(sb)           (S_ISDIR((sb)->st_mode))
#define msf_is_file(sb)          (S_ISREG((sb)->st_mode))
#define msf_is_link(sb)          (S_ISLNK((sb)->st_mode))
#define msf_is_exec(sb)          (((sb)->st_mode & S_IXUSR) == S_IXUSR)
#define msf_file_access(sb)      ((sb)->st_mode & 0777)
#define msf_file_size(sb)        (sb)->st_size
#define msf_file_fs_size(sb)     max((sb)->st_size, (sb)->st_blocks * 512)
#define msf_file_mtime(sb)       (sb)->st_mtime
/* inode number -inode节点号, 同一个设备中的每个文件, 这个值都是不同的*/  
#define msf_file_uniq(sb)        (sb)->st_ino  


s32 msf_set_file_time(u8 *name, s32 fd, time_t s);
s32 msf_create_file_mapping(struct msf_file_mapping *fm);
void msf_close_file_mapping(struct msf_file_mapping *fm);



/************************** DIR FUNC *********************************/
#define msf_realpath(p, r)       (u8 *) realpath((s8 *) p, (s8 *) r)
/* getcwd()会将当前工作目录的绝对路径复制到参数buf所指的内存空间中,
 * 参数size为buf的空间大小*/
#define msf_getcwd(buf, size)    (getcwd((s8 *) buf, size) != NULL)
#define msf_path_separator(c)    ((c) == '/')

#define MSF_MAX_PATH             PATH_MAX

#define MSF_DIR_MASK_LEN         0

s32 msf_open_dir(s8 *name, struct msf_dir *dir);
s32 ngx_read_dir(struct msf_dir *dir);



#define msf_create_dir(name, access) mkdir((const s8 *) name, access)

#define msf_close_dir(d)         closedir((d)->dir)

#define msf_delete_dir(name)     rmdir((const s8 *) name)

#define msf_dir_access(a)        (a | (a & 0444) >> 2)

#define msf_de_name(dir)         ((u8 *) (dir)->de->d_name)


#if (MSF_HAVE_D_NAMLEN)
#define msf_de_namelen(dir)      (dir)->de->d_namlen
#else
#define msf_de_namelen(dir)      strlen((dir)->de->d_name)
#endif

static inline s32 msf_de_info(u8 *name, struct msf_dir *dir)
{
    dir->type = 0;
    return stat((const s8 *) name, &dir->info);
}

#define msf_de_link_info(name, dir)  lstat((const s8 *) name, &(dir)->info)

#define msf_de_is_dir(dir)       (S_ISDIR((dir)->info.st_mode))
#define msf_de_is_file(dir)      (S_ISREG((dir)->info.st_mode))
#define msf_de_is_link(dir)      (S_ISLNK((dir)->info.st_mode))

#define msf_de_access(dir)       (((dir)->info.st_mode) & 0777)
#define msf_de_size(dir)         (dir)->info.st_size
#define msf_de_fs_size(dir)      max((dir)->info.st_size, (dir)->info.st_blocks * 512)
#define msf_de_mtime(dir)        (dir)->info.st_mtime


s32 msf_open_glob(struct msf_glob *gl);
s32 msf_read_glob(struct msf_glob *gl, s8 *name);
void msf_close_glob(struct msf_glob *gl);


s32 msf_trylock_fd(s32 fd);
s32 msf_lock_fd(s32 fd);
s32 msf_unlock_fd(s32 fd);


#if (MSF_HAVE_F_READAHEAD)

#define ngx_read_ahead(fd, n)    fcntl(fd, F_READAHEAD, (int) n)

#elif (MSF_HAVE_POSIX_FADVISE)

s32 ngx_read_ahead(s32 fd, size_t n);

#else

#define ngx_read_ahead(fd, n)    0

#endif



#if (MSF_HAVE_O_DIRECT)

s32 ngx_directio_on(s32 fd);
s32 ngx_directio_off(s32 fd);

#elif (MSF_HAVE_F_NOCACHE)

#define ngx_directio_on(fd)      fcntl(fd, F_NOCACHE, 1)

#elif (MSF_HAVE_DIRECTIO)

#define ngx_directio_on(fd)      directio(fd, DIRECTIO_ON)

#else

#define ngx_directio_on(fd)      0

#endif


size_t msf_fs_bsize(u8 *name);

void msf_enable_coredump(void);

