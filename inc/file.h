#include <time.h>

#define FILE_EXISTS 1
#define FILE_READ   2
#define FILE_EXEC   4


struct file_info {
    size_t size;
    time_t last_modification;

    /* Suggest flags to open this file */
    int flags_read_only;

    unsigned char exists;
    unsigned char is_file;
    unsigned char is_link;
    unsigned char is_directory;
    unsigned char exec_access;
    unsigned char read_access;
};

int   file_get_info(const char *path, struct file_info *f_info, int mode);
char* file_to_buffer(const char *path);

