
/*
 * Session logging.
 */
#include <stdarg.h>
#include <fcntl.h>
#include <msf_log.h>

#define MSF_MOD_LOGGER "MSF_LOG"

#define MSF_LOG(level, ...) \
    log_write(level, MSF_MOD_LOGGER, __func__, __FILE__, __LINE__, __VA_ARGS__)


#define LOG_FILE_PATH       "/home/luotang.me/log/rapberry.log"
#define LOG_FILE_ZIP        "/home/luotang.me/log/tomato-%ld.zip"

#define LOG_BUFFER_SIZE     4096

#define LOG_TITLE           "[MSF_LOG_INFO:%d]\n"
#define LOG_MAX_FILE_SIZE   (10*1024*1024) 
#define LOG_MAX_HEAD_LEN    32

static const s8 *loglevel[] = {
    "FATAL",
    "ERROR",
    "WARN",
    "DEBUG",
    "INFO",
};

static const s8 *logcolor[] = {
    RED,
    PURPLE,
    YELLOW,
    BROWN,
    BLUE,
    CYAN,
    GREEN,
    GRAY,
};

enum mod_id {
    MOD_LOG,
    MOD_SYS,
};

struct log_mod {
    s32 id;
    s32 level;
};

struct log_param {
    u8  enable;         /* log enable */
    u8  blogout;        /* log printf enable */
    u8  blogfile;       /* log file enable*/
    u8	blogredirect;   /* system srceen output */

    u8  logname[64];
    u32 logsize;        /* log limit size or log zip size */

    u8  bzip;           /* log zip enable */
    u8  zipalg;         /* log zip algorithm, lzma,tar,zip etc */
    u8  bencrypt;       /* log zip encrypt enable */
    u8  encryptalg;     /* log zip encrypt algorithm, md5,sha1,hash etc*/
    u8  bupload;        /* log zip upload to remote data center*/
    u8  bdownload;      /* log zip download from local system */
    u8  res1[2+16]; 
    /*u8    uploadurl[32];  log zip upload url */

    u8  bpcolor;        /* log printf color enable*/
    u8  bpfunc;         /* log printf func enable */
    u8  bpfile;         /* log printf file enable */
    u8  bpline;         /* log printf line number */

    u8  logstat;        /* log running state */
    u8  logver;         /* log version using bit set */
    u8  logmlevel;      /* min level of log */
    u8  res2[1+16];

    s32 logfd;          /* log thread/process pid */
    s32 logpid;         /* to avoid duplicates over a fork */

#ifdef  GLOG_SUPPORT

#endif

    pthread_mutex_t log_mutex;
};


static struct log_param log;
static struct log_param *lplog = &log;


static s32 log_current_time(s8 *localtime, u32 len) {
    struct tm tm;
    time_t t;

    if (unlikely(!localtime || len > 72)) {
    	return -1;
    }

    msf_memzero(&tm, sizeof(tm));

    time(&t);

    localtime_r( &t, &tm );

    snprintf(localtime, len - 1 , 
        "%d-%d-%d %02d:%02d:%02d",
        tm.tm_year + 1900,
        tm.tm_mon + 1, 
        tm.tm_mday,
        tm.tm_hour, 
        tm.tm_min, 
        tm.tm_sec);

    return 0;
}

s32 log_generate_name(s8 *name, u32 len) {

    if (unlikely(!name || len > 128)) {
        return -1;
    }

    time_t t;
    time(&t);

    snprintf(name, len - 1, LOG_FILE_ZIP, t);

    return 0;
}

static s8* log_dupvprintf(const s8 *fmt, va_list ap) {
    s8 *buf = NULL;
    s8 *prebuf  = NULL;
    s32 len, size = 0;

    buf = MSF_NEW(s8, 256);
    if (!buf) {
        return NULL;
    }

    size = 256;

    while (1) {

        #ifdef WIN32
        #define vsnprintf _vsnprintf
        #endif
        #ifdef va_copy
        /* Use the `va_copy' macro mandated by C99, if present.
         * XXX some environments may have this as __va_copy() */
        va_list aq;
        va_copy(aq, ap);
        len = vsnprintf(buf, size, fmt, aq);
        va_end(aq); 
        #else
      /* Ugh. No va_copy macro, so do something nasty.
       * Technically, you can't reuse a va_list like this: it is left
       * unspecified whether advancing a va_list pointer modifies its
       * value or something it points to, so on some platforms calling
       * vsnprintf twice on the same va_list might fail hideously
       * (indeed, it has been observed to).
       * XXX the autoconf manual suggests that using memcpy() will give
       *     "maximum portability". */

        len = vsnprintf(buf, size, fmt, ap);
        #endif
        if (len >= 0 && len < size) { 
           /* This is the C99-specified criterion for snprintf to have
            * been completely successful. */
            return buf;
        } else if (len > 0) { 
            /* This is the C99 error condition: the returned length is
             * the required buffer size not counting the NUL. */
            size = len + 1;
        } else {
            /* This is the pre-C99 glibc error condition: <0 means the
             * buffer wasn't big enough, so we enlarge it a bit and hope. */
            size += 256;
        }

        /*  If realloc failed, a null pointer is returned, and the memory 
         *  block pointed to by argument buf is not deallocated(not free or move!!!). 
         *  (it is still valid, and with its contents unchanged)
         *  ref: http://www.cplusplus.com/reference/cstdlib/realloc */
        prebuf = buf;
        buf = MSF_RENEW(buf, s8, size);
        if (!buf) {
            sfree(prebuf);
            return NULL;
        }
    }

}

static s32 log_update_file(struct log_param* ctx, s32 last_write_pos) {

    s32 ret = -1;
    s8  log_info[LOG_MAX_HEAD_LEN];

    if (!ctx || last_write_pos < 0) {
        return -1;
    }

    msf_memzero(log_info, sizeof(log_info));

    ret = lseek(ctx->logfd, 0, SEEK_SET); 
    if(ret < 0) {
         MSF_LOG(DBG_ERROR, "Lseek file start error:%s.\n", strerror(errno));
        return -1;
    } 
    snprintf(log_info, LOG_MAX_HEAD_LEN, LOG_TITLE, last_write_pos);
    ret  = write(ctx->logfd, log_info, strlen(log_info)); 
    if(ret != (int)strlen(log_info)) {
        MSF_LOG(DBG_ERROR, "Write file header error:%s.\n", strerror(errno));
        return -1;
    }
    lplog->logsize += last_write_pos;
    return 0;

}

static s32 log_write_file(struct log_param* ctx, const s8 *info, s32 len)
{ 
    if (!info || len <= 0) {
        MSF_LOG(DBG_ERROR, "Write file param is error.\n");
        return -1;
    } 

    if( !ctx || ctx->logfd < 0 ) {
        lplog->logstat 	= L_ERROR;
        return -1;
    }

    s32 ret = -1;  
    s32 total_len = 0; 
    s32 last_write_pos = 0;
    s8  read_buf[LOG_MAX_HEAD_LEN]; 

    msf_memzero(read_buf, sizeof(read_buf));

    total_len = lseek(ctx->logfd, 0, SEEK_END); 
    if(total_len < 0) {
         MSF_LOG(DBG_ERROR, "Lseek total_len error:%s.\n", strerror(errno));
        goto err;
    } else if(0 == total_len) {
        /* write header info */
        ret = log_update_file(ctx, LOG_MAX_HEAD_LEN);
        if(0 != ret) {
            goto err;
        }
        total_len += LOG_MAX_HEAD_LEN;
        ret = lseek(ctx->logfd, LOG_MAX_HEAD_LEN, SEEK_SET);
        if (ret < 0) {
            MSF_LOG(DBG_ERROR, "Lseek log_head_len failed, errno(%d).\n", errno);
            goto err;
        }
    }
    else {
    /* jump to file start */
    ret = lseek(ctx->logfd, 0, SEEK_SET); 
    if (ret < 0) {
        MSF_LOG(DBG_ERROR, "Lseek file start error:%s\n", strerror(errno));
        goto err;
    }

    /* check whether the log file size > LOG_FILE_SIZE,
        device maybe reboot*/
    ret = read(ctx->logfd, read_buf, LOG_MAX_HEAD_LEN);
    if ( ret < 0 ) { 
        MSF_LOG(DBG_ERROR, "Read info failed(%d-%d)\n", ret, len);
        goto err;
    }

    sscanf(read_buf, "[MSF_LOG_INFO:%d]", &last_write_pos);

    //printf("log last_write_pos:%d\n", last_write_pos);

    if (last_write_pos + len > LOG_MAX_FILE_SIZE) {
        MSF_LOG(DBG_ERROR, "Write log roolback.\n"); 
        last_write_pos = LOG_MAX_HEAD_LEN;
    }
    /* Skip log header*/
    ret = lseek(ctx->logfd, last_write_pos, SEEK_SET); 
    if (ret < 0) { 
        MSF_LOG(DBG_ERROR, "Lseek last_write_pos errno(%d).\n", errno);
        goto err;
    }
    total_len = last_write_pos;
    }

    ret = write(ctx->logfd, info, len); 
    if(ret != len){  
        MSF_LOG(DBG_ERROR, "Lseek write log errno(%d).\n", errno);
        goto err;
    }

    ret = log_update_file(ctx, total_len + len);
    if(ret != 0) {
        goto err;
    }

    fsync(ctx->logfd);
    return 0;

    err:
    fsync(ctx->logfd);
    return -1;
}


s32 log_init(const s8 *log_path) {

    s8 log_name[128];

    msf_memzero(log_name, sizeof(log_name));
    msf_memzero(lplog, sizeof(struct log_param));

    lplog->enable       = 1;
    lplog->blogout      = 1;
    lplog->blogfile     = 1;
    lplog->blogredirect = 1;

    lplog->bzip         = 1;
    lplog->zipalg       = 0;
    lplog->bencrypt     = 0;
    lplog->encryptalg   = 0;
    lplog->bupload      = 0;
    lplog->bdownload    = 0;

    lplog->bpcolor      = false;
    lplog->bpfile       = false;
    lplog->bpfunc       = true;
    lplog->bpline       = true;

    lplog->logfd        = -1;
    lplog->logver       = 1;
    lplog->logpid       = getpid();
    lplog->logstat      = L_CLOSED;
    lplog->logmlevel    = DBG_INFO;
    lplog->logsize      = LOG_MAX_FILE_SIZE;

    pthread_mutex_init( &(lplog->log_mutex), NULL );

    lplog->logstat  = L_OPENING;

    if(lplog->blogfile) {
        lplog->logfd = open(log_path, 
            O_CREAT | O_RDWR | O_APPEND , 0766); 
        if (lplog->logfd < 0) {
            fprintf(stderr, "Fail to open log file(%s).\n", log_path);
            lplog->logstat = L_ERROR;
            return -1;
        }
    }
#ifdef GLOG_SUPPORT
    if(lplog->bglogfile) {
        log_generate_name(log_name, sizeof(log_name));
        lplog->lgzlog = gzlog_open(log_name);
    }
#endif


    lplog->logstat  = L_OPEN;

    return 0;
}


void log_free(void) {
    lplog->logstat = L_CLOSED;
    sclose(lplog->logfd);

    return;
}

s32 log_zip(void) {

    FILE*  file = NULL;
    u32    clen = 0;
    s32    ret  = -1;

    u8*  cbuf = NULL; 

    s8 log_name[128];
    s8 cmd[256];

    msf_memzero(cmd, sizeof(cmd));
    msf_memzero(log_name, sizeof(log_name));

    log_generate_name(log_name, sizeof(log_name));

    return 0;
}

#define log_format_default "[%s][%s][%s][%s:%d] %s"

s32 log_write(s32 level, s8 *mod, const s8 *func, const s8 *file, s32 line, s8 *fmt, ... ) {

    pthread_mutex_lock( &(lplog->log_mutex));

    s32 ret = -1;
    va_list ap;
    s8 *data = NULL; /* save the param */

    s8 tmfmt[256];
    s8 log_name[128];
    s8 newfmt[512];

    msf_memzero(tmfmt, sizeof(tmfmt));
    msf_memzero(log_name, sizeof(log_name));
    msf_memzero(newfmt, sizeof(newfmt));

    if (unlikely(!mod || !fmt)) {
        goto err;
    }

    {
    va_start(ap, fmt);

    ret = log_current_time(tmfmt, 72);
    if(ret != 0) {
         fprintf(stderr, "Fail to get time %s.\n", tmfmt);
        goto err;
    }
    
    if (lplog->bpcolor && lplog->bpfile && 
        lplog->bpline && lplog->bpfunc ) {

        snprintf(newfmt, sizeof(newfmt) - 1,
            "[%s]%s[%s][%s][%s %s:%d]:" NONE YELLOW " %s",
            tmfmt, 
            logcolor[level],
            mod,
            loglevel[level],
            func,
            file,
            line,
            fmt);
    } else {
        snprintf(newfmt, sizeof(newfmt) - 1,
            "[%s][%s][%s][%s %s:%d] %s",
            tmfmt,
            mod,
            loglevel[level],
            func,
            file,
            line,
            fmt);

    }

    data = log_dupvprintf(newfmt, ap);
    if (!data) {
        fprintf(stderr, "Fail to dupvp buffer %s, errno(%d).\n",
               tmfmt, errno);
        goto err;
    }
    va_end(ap);

    }

    /*
    * In state L_CLOSED, we call logfopen, which will set the state
    * to one of L_OPENING, L_OPEN or L_ERROR. Hence we process all of
    * those three _after_ processing L_CLOSED.
    */
   // if (unlikely(lplog->logstat == L_CLOSED))
    //    log_init();

    if (lplog->logstat == L_OPENING || lplog->logstat == L_ZIPING) {
        //bufchain_add(&lplog->queue, data, strlen(data));
    } else if (lplog->logstat == L_OPEN) {
    
        if (lplog->blogout && level < lplog->logmlevel) {
            fprintf(stderr, "%s\n", data);
        }
        if (lplog->blogfile) {
            ret = log_write_file(lplog, data, strlen(data));
            if(ret != 0) {
                fprintf(stderr, "Fail to write buffer %s.\n", tmfmt);
                goto err;
            }
        }

#ifdef GLOG_SUPPORT
	
#endif
        sfree(data);
        if(lplog->bzip) {
            if(lplog->logsize >= LOG_MAX_FILE_SIZE) {

                lplog->logstat = L_ZIPING;

                log_zip();

                /* empty log file */
                ftruncate(lplog->logfd, 0);
                lseek(lplog->logfd, 0, SEEK_SET);
                lplog->logsize = 0;
                lplog->logstat = L_OPEN;
            }
        }
    } /* else L_ERROR, so ignore the write */

    pthread_mutex_unlock( &(lplog->log_mutex));
    return 0;
err:
    lplog->logstat = L_ERROR;
    sfree(data);
    pthread_mutex_unlock( &(lplog->log_mutex));
    return -1;
}

