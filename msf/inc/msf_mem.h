/**************************************************************************
*
* Copyright (c) 2017-2019, luotang.me <wypx520@gmail.com>, China.
* All rights reserved.
*
* Distributed under the terms of the GNU General Public License v2.
*
* This software is provided 'as is' with no explicit or implied warranties
* in respect of its properties, including, but not limited to, correctness
* and/or fitness for purpose.
*
**************************************************************************/

#include <msf_list.h>
#include <msf_file.h>
#include <msf_network.h>
#include <msf_atomic.h>
#include <msf_thread.h>

#include <numa.h> /*debian-ubuntu:apt-get install libnuma-dev*/
#include <sys/mman.h>
#include <sys/syscall.h>

extern s32 get_cpu_mhz(s32 cpu);

#define HUGE_PAGE_SZ    (2*1024*1024)

#define msf_sfree(ptr) do { \
        if ((ptr) != NULL) { \
            free((ptr)); \
            (ptr) = NULL;} \
        } while(0) 

static MSF_ALWAYS_INLINE long msf_get_page_size(void) {

    static long page_size;

    if (!page_size)
        page_size = sysconf(_SC_PAGESIZE);

    return page_size;
}

static inline s32 msf_memalign(void **memptr, size_t alignment, size_t size) {
    return posix_memalign(memptr, alignment, size);
}

static inline void *msf_mmap(size_t length) {
    return mmap(NULL, length, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS |
            MAP_POPULATE | MAP_HUGETLB, -1, 0);
}

static inline s32 msf_munmap(void *addr, size_t length) {
    return munmap(addr, length);
}

static inline void *msf_numa_alloc_onnode(size_t size, s32 node) {
    return numa_alloc_onnode(size, node);
}

static inline void msf_numa_free(void *start, size_t size) {
    numa_free(start, size);
}


/* CPU */
static inline long msf_get_num_processors(void) {

    static long num_processors;

    if (!num_processors)
        num_processors = sysconf(_SC_NPROCESSORS_CONF);

    return num_processors;
}

static inline int msf_clock_gettime(struct timespec *ts) {
    return clock_gettime(CLOCK_MONOTONIC, ts);
}

static inline s32 msf_numa_node_of_cpu(s32 cpu) {
    return numa_node_of_cpu(cpu);
}

static inline s32 msf_numa_run_on_node(s32 node) {
    return numa_run_on_node(node);
}

#define MSF_HZ_DIR   "/var/tmp/msf.d"
#define MSF_HZ_FILE  MSF_HZ_DIR "/hz"

/* since this operation may take time cache it on a cookie,
 * and use the cookie if exist*/
static inline double msf_get_cpu_mhz(void) {
    s8  size[32] = {0};
    double  hz = 0;
    s32 fd;
    ssize_t ret;

    fd = open(MSF_HZ_FILE, O_RDONLY);
    if (fd < 0)
        goto try_create;

    ret = read(fd, size, sizeof(size));

    close(fd);

    if (ret > 0)
        return atof(size);

    try_create:
    hz = get_cpu_mhz(0);

    ret = mkdir(MSF_HZ_DIR, 0777);
    if (ret < 0)
        goto exit;

    fd = open(MSF_HZ_FILE, O_CREAT | O_TRUNC | O_WRONLY | O_SYNC, 0644);
    if (fd < 0)
        goto exit;

    sprintf(size, "%f", hz);
    ret = write(fd, size, sizeof(size));
    if (ret < 0)
        goto close_and_exit;

close_and_exit:
    close(fd);
exit:
    return hz;
}

static inline s32 msf_pin_to_node(s32 cpu) {
    s32 node = msf_numa_node_of_cpu(cpu);
    /* pin to node */
    s32 ret = msf_numa_run_on_node(node);

    if (ret)
        return -1;

    /* is numa_run_on_node() guaranteed to take effect immediately? */
    msf_sched_yield();

    return -1;
}

/*
 *  Determine whether some value is a power of two, where zero is
 * *not* considered a power of two.
 */

static inline __attribute__((const))
s32 is_power_of_2(unsigned long n) {
    return (n != 0 && ((n & (n - 1)) == 0));
}


static s32 intf_numa_node(const s8 *iface) {
    s32 fd, numa_node = -1, len;
    s8  buf[256];

    snprintf(buf, 256, "/sys/class/net/%s/device/numa_node", iface);
    fd = open(buf, O_RDONLY);
    if (fd == -1)
        return -1;

    len = read(fd, buf, sizeof(buf));
    if (len < 0)
        goto cleanup;

    numa_node = strtol(buf, NULL, 0);

cleanup:
    close(fd);
    return numa_node;
}


void *msf_malloc_huge_pages(size_t size) {
    s32 retval;
    size_t  real_size;
    void    *ptr = NULL;

#ifndef WIN32
    long page_size = msf_get_page_size();

    if (page_size < 0) {
        printf("sysconf failed. (errno=%d %m)\n", errno);
        return NULL;
    }

    real_size = msf_align(size, page_size);
    retval = msf_memalign(&ptr, page_size, real_size);
    if (retval) {
        printf("posix_memalign failed sz:%zu. %s\n",
             real_size, strerror(retval));
        return NULL;
    }
    memset(ptr, 0, real_size);
    return ptr;
#endif
    /* Use 1 extra page to store allocation metadata */
    /* (libhugetlbfs is more efficient in this regard) */
    real_size = msf_align(size + HUGE_PAGE_SZ, HUGE_PAGE_SZ);

    ptr = msf_mmap(real_size);
    if (!ptr || ptr == MAP_FAILED) {
        /* The mmap() call failed. Try to malloc instead */
        long page_size = msf_get_page_size();

        if (page_size < 0) {
             printf("sysconf failed. (errno=%d %m)\n", errno);
            return NULL;
        }
        printf("huge pages allocation failed, allocating " \
             "regular pages\n");

         printf("mmap rdma pool sz:%zu failed (errno=%d %m)\n",
          real_size, errno);
        real_size = msf_align(size + HUGE_PAGE_SZ, page_size);
        retval = msf_memalign(&ptr, page_size, real_size);
        if (retval) {
             printf("posix_memalign failed sz:%zu. %s\n",
              real_size, strerror(retval));
            return NULL;
        }
        memset(ptr, 0, real_size);
        real_size = 0;
    } else {
        printf("Allocated huge page sz:%zu\n", real_size);
    }
    /* Save real_size since mmunmap() requires a size parameter */
    *((size_t *)ptr) = real_size;
    /* Skip the page with metadata */
    return (s8*)ptr + HUGE_PAGE_SZ;
}


void msf_free_huge_pages(void *ptr) {
    void    *real_ptr;
    size_t  real_size;

    if (!ptr)
        return;

#ifndef WIN32
        sfree(ptr);
        return;
#endif

    /* Jump back to the page with metadata */
    real_ptr = (s8 *)ptr - HUGE_PAGE_SZ;
    /* Read the original allocation size */
    real_size = *((size_t *)real_ptr);

    if (real_size != 0)
        /* The memory was allocated via mmap()
           and must be deallocated via munmap()
           */
        msf_munmap(real_ptr, real_size);
    else
        /* The memory was allocated via malloc()
           and must be deallocated via free()
           */
        sfree(real_ptr);
}


void *msf_numa_alloc(size_t bytes, s32 node) {

    if (numa_available() < 0) { 
        printf("Your system does not support NUMA API\n"); 
    } 

    long page_size = msf_get_page_size();
    size_t real_size = msf_align((bytes + page_size), page_size);
    void *p = msf_numa_alloc_onnode(real_size, node);

    if (!p) {
        printf("numa_alloc_onnode failed sz:%zu. %m\n",
              real_size);
        return NULL;
    }
    /* force the OS to allocate physical memory for the region */
    memset(p, 0, real_size);

    /* Save real_size since numa_free() requires a size parameter */
    *((size_t *)p) = real_size;

    /* Skip the page with metadata */
    return (s8*)p + page_size;
}

void msf_numa_free_ptr(void *ptr) {

    void    *real_ptr;
    size_t  real_size;

    if (!ptr) return;

    long page_size = msf_get_page_size();

    /* Jump back to the page with metadata */
    real_ptr = (char *)ptr - page_size;
    /* Read the original allocation size */
    real_size = *((size_t *)real_ptr);

    if (real_size != 0)
        /* The memory was allocated via numa_alloc()
           and must be deallocated via numa_free()
           */
        msf_numa_free(real_ptr, real_size);
    }


/********************Interface NUMA**************************************/
static s32 numa_node_to_cpusmask(int node, u64 *cpusmask, s32 *nr) {
    struct bitmask *mask;
    u64    bmask = 0;
    s32 retval = -1;
    u32 i;

    mask = numa_allocate_cpumask();
    retval = numa_node_to_cpus(node, mask);
    if (retval < 0)
        goto cleanup;

    *nr = 0;
    for (i = 0; i < mask->size && i < 64; i++) {
        if (numa_bitmask_isbitset(mask, i)) {
            msf_set_bit(i, &bmask);
            (*nr)++;
        }
    }

    retval = 0;
cleanup:
    *cpusmask = bmask;

    numa_free_cpumask(mask);
    return retval;
}

static s32 intf_name_best_cpus(const s8 *if_name, u64 *cpusmask, s32 *nr) {
    int numa_node, retval;

    *cpusmask = 0;
    numa_node = intf_numa_node(if_name);
    if (numa_node < 0)
        return -1;

    retval = numa_node_to_cpusmask(numa_node, cpusmask, nr);

    return retval;
}
static s8 *intf_cpusmask_str(u64 cpusmask, s32 nr, s8 *str) {
    s32 len = 0, i, cpus;

    for (i = 0, cpus = 0; i < 64 && cpus < nr; i++) {
        if (msf_test_bit(i, &cpusmask)) {
            len += sprintf(&str[len], "%d ", i);
            cpus++;
        }
    }
    return str;
}
static s32 intf_master_name(const s8 *iface, s8 *master)
{
    s32 fd, len;
    s8 path[256];
    s8 buf[256];
    s8    *ptr;

    snprintf(path, 256, "/sys/class/net/%s/master", iface);
    fd = open(path, O_RDONLY);
    if (fd == -1)
        return -1;

    len = read(fd, buf, sizeof(buf) - 1);
    if (len < 0) {
        len = readlink(path, buf, sizeof(buf) - 1);
        if (len < 0)
            goto cleanup;
    }
    buf[len] = '\0';
    ptr = strrchr(buf, '/');
    if (ptr) {
        ptr++;
        strcpy(buf, ptr);
    }
    strcpy(master, buf);
cleanup:
    close(fd);

    return (len > 0) ? 0 : -1;
}

static s32 msf_intf_main(s32 argc, s8 *argv[]) 
{
    struct ifaddrs  *ifaddr, *ifa;
    s8  host[NI_MAXHOST] = {0};
    s8  cpus_str[256];
    s8  flags[1024];
    u64 cpusmask = 0;
    s32 cpusnum;
    s32 retval = -1;
    s32 ec = EXIT_FAILURE;
    s32 numa_node;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        goto cleanup;
    }
    printf("%-10s %-16s %-30s %-5s %-10s %-40s\n",
           "interface", "host", "flags", "numa", "cpus mask", "cpus");
    printf("---------------------------------------------------");
    printf("-------------------------------------------------------\n");

    for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
        switch (ifa->ifa_addr->sa_family) {
        case AF_INET:
            if (!(ifa->ifa_flags & IFF_UP))
            continue;
            getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                    host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
        break;
        case AF_PACKET:
            if (ifa->ifa_flags & IFF_MASTER)
                continue;
            if (ifa->ifa_flags & IFF_SLAVE)
                break;
            if (!(ifa->ifa_flags & IFF_UP))
                break;
            continue;
            break;
        default:
            continue;
            break;
        }
        flags[0] = 0;
        s8 *if_up= NULL;
        s8 *if_loop= NULL;
        s8 *if_master= NULL;
        
        if (ifa->ifa_flags & IFF_UP)
            if_up = "UP";
        else
            if_up = "DOWN";
        
        if (ifa->ifa_flags & IFF_LOOPBACK)
            if_loop = "LOOPBACK";
        if (ifa->ifa_flags & IFF_RUNNING)
            if_loop = "RUNNING";
        if (ifa->ifa_flags & IFF_SLAVE) {
            s8 master[256];

            intf_master_name(ifa->ifa_name, master);
            if_master = "SLAVE";
            sprintf(flags, "%s %s %s - [%s]", 
                if_up, if_loop, if_master, master);
        }
        
        if (ifa->ifa_flags & IFF_MASTER) {
            if_master = "MASTER";
            sprintf(flags, "%s %s %s", 
                if_up, if_loop, if_master);
        }

        numa_node = intf_numa_node(ifa->ifa_name);
        retval = intf_name_best_cpus(ifa->ifa_name,
                     &cpusmask, &cpusnum);
        if (retval != 0) {
            /*perror("intf_name_best_cpus"); */
            printf("%-10s %-16s %-30s %-5c 0x%-8lx %-4s[0]\n",
                   ifa->ifa_name, host, flags, 0x20, 0UL, "cpus");
            continue;
        }
        intf_cpusmask_str(cpusmask, cpusnum, cpus_str);

        printf("%-10s %-16s %-30s %-5d 0x%-8lx %-4s[%d] - %s\n",
               ifa->ifa_name, host, flags, numa_node, cpusmask,
               "cpus",  cpusnum, cpus_str);
    }
    ec = EXIT_SUCCESS;

    freeifaddrs(ifaddr);

cleanup:
    exit(ec);
}


