#include "file.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>


int file_get_info(const char *path, struct file_info *f_info, int mode)
{
    struct stat f, target;

    f_info->exists = 0;

    /* Stat right resource */
    if (lstat(path, &f) == -1) {
        if (errno == EACCES) {
            f_info->exists = 1;
        }
        return -1;
    }

    f_info->exists = 1;
    f_info->is_file = 1;
    f_info->is_link = 0;
    f_info->is_directory = 0;
    f_info->exec_access = 0;
    f_info->read_access = 0;

    if (S_ISLNK(f.st_mode)) {
        f_info->is_link = 1;
        f_info->is_file = 0;
        if (stat(path, &target) == -1) {
            return -1;
        }
    }
    else {
        target = f;
    }

    f_info->size = target.st_size;
    f_info->last_modification = target.st_mtime;

    if (S_ISDIR(target.st_mode)) {
        f_info->is_directory = 1;
        f_info->is_file = 0;
    }

#ifndef _WIN32
    gid_t EGID = getegid();
    gid_t EUID = geteuid();

    /* Check read access */
    if (mode & FILE_READ) {
        if (((target.st_mode & S_IRUSR) && target.st_uid == EUID) ||
            ((target.st_mode & S_IRGRP) && target.st_gid == EGID) ||
            (target.st_mode & S_IROTH)) {
            f_info->read_access = 1;
        }
    }

    /* Checking execution */
    if (mode & FILE_EXEC) {
        if ((target.st_mode & S_IXUSR && target.st_uid == EUID) ||
            (target.st_mode & S_IXGRP && target.st_gid == EGID) ||
            (target.st_mode & S_IXOTH)) {
            f_info->exec_access = 1;
        }
    }
#endif

    /* Suggest open(2) flags */
    f_info->flags_read_only = O_RDONLY | O_NONBLOCK;

#if defined(__linux__)
    /*
     * If the user is the owner of the file or the user is root, it
     * can set the O_NOATIME flag for open(2) operations to avoid
     * inode updates about last accessed time
     */
    if (target.st_uid == EUID || EUID == 0) {
        f_info->flags_read_only |=  O_NOATIME;
    }
#endif

    return 0;
}

/* Read file content to a memory buffer,
 * Use this function just for really SMALL files
 */
char* file_to_buffer(const char *path)
{
    FILE *fp = NULL;
    char *buffer = NULL;
    long bytes;
    struct file_info finfo;

    if (file_get_info(path, &finfo, FILE_READ) != 0) {
        return NULL;
    }

    if (!(fp = fopen(path, "r"))) {
        return NULL;
    }

    buffer = (char *)malloc(finfo.size + 1);
    if (!buffer) {
        fclose(fp);
        return NULL;
    }

    bytes = fread(buffer, finfo.size, 1, fp);

    if (bytes < 1) {
        free(buffer);
        fclose(fp);
        return NULL;
    }

    fclose(fp);
    return (char *) buffer;

}


