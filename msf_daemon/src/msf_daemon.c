/**************************************************************************
*
* Copyright (c) 2017-2018, luotang.me <wypx520@gmail.com>, China.
* All rights reserved.
*
* Distributed under the terms of the GNU General Public License v2.
*
* This software is provided 'as is' with no explicit or implied warranties
* in respect of its properties, including, but not limited to, correctness
* and/or fitness for purpose.
*
**************************************************************************/
#include <msf.h>
#include <cJSON.h>

#define DEF_MSF_DAEMON_NAME "msf_daemon"
#define DEF_MSF_DAEMON_CONFIG_PATH "/home/luotang.me/msf_daemon_conf.json"

#define DEF_MSF_KEY_AUTHOR      "msf_author"
#define DEF_MSF_KEY_VERSION     "msf_version"
#define DEF_MSF_KEY_DESC        "msf_desc"
#define DEF_MSF_KEY_PROCS       "msf_procs"

#define DEF_PROC_KEY_IDX        "proc_idx"
#define DEF_PROC_KEY_NAME       "proc_name"
#define DEF_PROC_KEY_PATH       "proc_path"
#define DEF_PROC_KEY_CONF       "proc_conf"


static struct msf_daemon gmsf;
struct msf_daemon *g_msf = &gmsf;

extern s8 *optarg;
extern s32 optind;
extern s32 opterr;
extern s32 optopt;

void daemon_show_version(void) {

    u32 proc_idx;
    struct svcinst *svc = NULL;
    struct process_desc *proc_desc = NULL;

    printf("### Micro service framework debug info ###\n\n");

    printf("### Msf author = (%s)\n", g_msf->msf_author);
    printf("### Msf version = (%s)\n", g_msf->msf_version);
    printf("### Msf desc = (%s)\n\n", g_msf->msf_desc);

    printf("### Msf proc_num = (%d)\n\n", g_msf->msf_proc_num);

    for (proc_idx = 0; proc_idx < g_msf->msf_proc_num; proc_idx++) {
        proc_desc = &(g_msf->msf_proc_desc[proc_idx]);		
        printf("### Msf proc_idx = (%d)\n", proc_desc->proc_idx);
        printf("### Msf proc_name = (%s)\n", proc_desc->proc_name);
        printf("### Msf proc_path = (%s)\n", proc_desc->proc_path);
        printf("### Msf proc_conf = (%s)\n\n", proc_desc->proc_conf);
    }

    printf("### Micro service framework debug info ###\n");

}


void daemon_show_usage(void) {
    fprintf(stderr,
            "Usage: %s [-?hvdc] [-d level] [-c config-file] [-k signal]\n"
            "       -h        Print help message.\n"
            "       -v        Show Version and exit.\n"
            "       -d        daemon mode.\n"
            "       -c file   Use given config-file instead of\n"
            "                 %s\n"
            "       -k reload|rotate|kill|parse\n"
            "                 kill is fast shutdown\n"
            "                 Parse configuration file, then send signal to \n"
            "                 running copy (except -k parse) and exit.\n",
            DEF_MSF_DAEMON_NAME, g_msf->msf_conf);
    exit(1);
}

s32 daemon_options_parse(s32 argc, s8 *argv[]) {
    s32 c;

    printf("optind:%d, opterr: %d\n", optind, opterr);

    //getopt_long
    while ((c = getopt(argc, argv, "hvdc:k:?")) != -1) {
        switch (c) {
            case 'h':
                daemon_show_usage();
                break;
            case 'v':
                daemon_show_version();
                break;
            case 'd':
                break;
            case 'c':
                g_msf->msf_conf = strdup(optarg);
                break;
            case 'k':
                break;
            case '?':
            default:
               printf("Unknown option: %c\n",(s8)optopt);
                daemon_show_usage();
                break;
        }
    }

    return 0;
}

static s32 daemon_parse_one_proc(cJSON *proc_array, s32 proc_idx) {

    cJSON *tmp = NULL;
    cJSON *proc_node = NULL;
    struct process_desc *proc_desc = NULL;

    proc_desc = &(g_msf->msf_proc_desc[proc_idx]);
    proc_node = cJSON_GetArrayItem(proc_array, proc_idx);
    if (!proc_node) {
        goto cleanup;
    }

    tmp = cJSON_GetObjectItem(proc_node, DEF_PROC_KEY_IDX);
    if (tmp) {
        proc_desc->proc_idx =  tmp->valueint;
    } else {
        goto cleanup;
    }

    tmp = cJSON_GetObjectItem(proc_node, DEF_PROC_KEY_NAME);
    if (tmp) {
        memcpy(proc_desc->proc_name, tmp->valuestring, 
            min(sizeof(proc_desc->proc_name)-1, strlen(tmp->valuestring)));
    } else {
        goto cleanup;
    }

    tmp = cJSON_GetObjectItem(proc_node, DEF_PROC_KEY_PATH);
    if (tmp) {
        memcpy(proc_desc->proc_path, tmp->valuestring, 
            min(sizeof(proc_desc->proc_path)-1, strlen(tmp->valuestring)));
    } else {
    	goto cleanup;
    }

    tmp = cJSON_GetObjectItem(proc_node, DEF_PROC_KEY_CONF);
    if (tmp) {
        memcpy(proc_desc->proc_conf, tmp->valuestring, 
            min(sizeof(proc_desc->proc_conf)-1, strlen(tmp->valuestring)));
    } else {
        goto cleanup;
    }
    return 0;

cleanup:
	return -1;
}

static s32 daemon_parse_all_proc(const s8 *confbuff) {

    s32 rc;
    u32 proc_idx;
    cJSON *tmp = NULL;
    cJSON *root = NULL;
    cJSON *proc_array = NULL;

    root = cJSON_Parse(confbuff);
    if (!root) {
        printf("Failed to parse %s.\n", DEF_MSF_DAEMON_CONFIG_PATH);
        goto cleanup;
    }

    tmp = cJSON_GetObjectItem(root, DEF_MSF_KEY_AUTHOR);
    if (tmp) {
        memcpy(g_msf->msf_author, tmp->valuestring, 
            min(sizeof(g_msf->msf_author)-1, strlen(tmp->valuestring)));
    } else {
        goto cleanup;
    }

    tmp = cJSON_GetObjectItem(root, DEF_MSF_KEY_VERSION);
    if (tmp) {
        memcpy(g_msf->msf_version, tmp->valuestring, 
            min(sizeof(g_msf->msf_version)-1, strlen(tmp->valuestring)));
    } else {
        goto cleanup;
    }

    tmp = cJSON_GetObjectItem(root, DEF_MSF_KEY_DESC);	
    if (tmp) {
        memcpy(g_msf->msf_desc, tmp->valuestring, 
            min(sizeof(g_msf->msf_desc)-1, strlen(tmp->valuestring)));
    } else {
        goto cleanup;
    }

    proc_array = cJSON_GetObjectItem(root, DEF_MSF_KEY_PROCS);
    if (proc_array) {
        g_msf->msf_proc_num = cJSON_GetArraySize(proc_array);		
    } else {
        goto cleanup;
    }

    if (0 == g_msf->msf_proc_num) {
        goto cleanup;
    }

    g_msf->msf_proc_desc = MSF_NEW(struct process_desc, g_msf->msf_proc_num);
    if (!g_msf->msf_proc_desc) {
        goto cleanup;
    } else {
        memset(g_msf->msf_proc_desc, 0, 
            sizeof(struct process_desc) * g_msf->msf_proc_num);
    }

    for (proc_idx = 0; proc_idx < g_msf->msf_proc_num; proc_idx++) {		
        rc = daemon_parse_one_proc(proc_array, proc_idx);
        if (rc < 0) {
            sfree(g_msf->msf_proc_desc);
            goto cleanup;
        }
    }
    return 0;

cleanup:
    cJSON_Delete(root);
    return rc;
}


s32 daemon_config_init(void) {

    s8 *confbuff = NULL;

    confbuff = config_read_file(DEF_MSF_DAEMON_CONFIG_PATH);
    if (!confbuff) return -1;

    if (daemon_parse_all_proc(confbuff) < 0) {
        sfree(confbuff);
        return -1;
    }

    sfree(confbuff);
    return 0;
}

void * daemon_worker_thread(void *param) {

    while (1) {

    }

    return NULL;
}

s32 daemon_process_init(void) {

    u32 proc_idx;
    struct process_desc *proc_desc = NULL;

    memset(g_msf, 0, sizeof(struct msf_daemon));

    if (daemon_config_init() < 0) {
        printf("Daemon config init failed.\n");
        sfree(g_msf->msf_proc_desc);
        return -1;
    }

    for (proc_idx = 0; proc_idx < g_msf->msf_proc_num; proc_idx++) {
        proc_desc = &(g_msf->msf_proc_desc[proc_idx]);
        printf("Process init name %s.\n",  proc_desc->proc_name);
        process_spwan(proc_desc);
    }

    return 0;
}


/* ./msf_kernel -c msf_daemon_config.json */
s32 main(s32 argc, s8 *argv[]) {

    if (unlikely((3 != argc && 2 != argc && 1 != argc) 
    || (3 == argc && !argv[2]))) {
        return -1;
    }

    if (daemon_options_parse(argc, argv) < 0) {
        return -1;
    }

    if (daemon_process_init() < 0) {
        return -1;
    }

    daemon_show_version();

    for ( ;; ) {
    //process_wait_child_termination();
        sleep(1);
    }

    printf("Process daemon exit.....\n");

    return 0;
}

