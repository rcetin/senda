#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <json-c/json.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>

#include <net/if.h>

#include "debug/debug.h"
#include "utils/utils.h"
#include "eth/ethersend.h"
#include "udp/udpsend.h"
#include "tcp/tcpsend.h"
#include "config/config.h"
#include "main.h"

static void print_help(void);
static void show_current_opts(void);
static void destroy_runtime_configs(void);
static void cleanup_main_thread(void);

static streamtype_e get_protocol_from_string(const char *str)
{
    if (!strncmp(str, "tcp", 3)) return TCP;
    else if (!strncmp(str, "udp", 3)) return UDP;
    else return ETH;
}

void print_help(void)
{
    fprintf(stderr, "senda - a network packet sender\n\n"
            "Usage:\n"
            "--------------------------------------------------------------------------\n"
            "-d, --dst [MAC]        Destination MAC\n"
            "-i, --if [ifname]      Interface name\n"
            "-b, --data [data]      Binary data\n"
            "-r, --rand [bytes]     Random data with given size\n"
            "-c, --count [count]    Count to send a packet (optional, default infinite)\n"
            "-t, --interval [interval]  Packet interval (optional, default 1s)\n"
            "-l, --log              set log level\n"
            "-o, --opt              get current options\n"
            "-s, --sender           sender type (\"tcp\", \"udp (default)\", \"eth\")\n"
            "-a, --addr             IP address (necessary if sender is udp or tcp)\n"
            "-p, --port             port (necessary if sender is udp or tcp)\n");
}

void show_current_opts(void)
{
    fprintf(stderr, "senda - a network packet sender\n\n"
    "Options:\n"
    "log: %s\n", debug2str(get_debug_level()));
}

static struct option long_options[] = {
    {"dst",     required_argument, 0,  'd' },
    {"if",     required_argument, 0,  'i' },
    {"data",    required_argument, 0,  'b' },
    {"rand",    required_argument, 0,  'r' },
    {"count",   required_argument, 0,  'c' },
    {"interval",  required_argument, 0, 't'},
    {"opt",  no_argument, 0, 'o'},
    {"sender",  required_argument, 0, 's'},
    {"addr",  required_argument, 0, 'a'},
    {"port",  required_argument, 0, 'p'},
    {0,         0,                 0,  0 }
};

typedef struct single_mode_cfg {
    streamtype_e protocol;
    uint8_t *data;
    uint32_t datalen;
    uint32_t interval_ms;
    uint32_t count;
    void *ctx;
    void *sender_handle;
} single_mode_cfg_t;

struct cfg_node {
    TAILQ_ENTRY(cfg_node) node;
    single_mode_cfg_t *cfg;
    pthread_t *thread_data;
};
TAILQ_HEAD(configq, cfg_node);

struct configq g_configs;

struct global_sender {
    char name[16];
    struct sender *worker;
    void *ctx;
};

struct global_sender sender_dispatcher[TOTAL_SENDER] = {
    {.name = "udp", .worker = &udpsender},
    {.name = "eth", .worker = &ethsender},
    {.name = "tcp", .worker = &tcpsender},
};

static void sigint_handler(int sig)
{
    struct cfg_node *cnode = NULL;
    int ret;
    void *sender_handle = NULL;

    TAILQ_FOREACH(cnode, &g_configs, node) {
        ret = pthread_cancel(*cnode->thread_data);
        if (ret) {
            errorf("[SIGINT] thread cancel failed");
        }

        ret = pthread_join(*cnode->thread_data, NULL);
        if (ret) {
            errorf("thread join failed");
        }

        sender_handle = cnode->cfg->sender_handle;
        sender_dispatcher[cnode->cfg->protocol].worker->destroy(sender_handle);
    }

    cleanup_main_thread();
    exit(EXIT_SUCCESS);
}

static void cleanup_main_thread(void)
{
    destroy_runtime_configs();
    config_destroy();
}

static int join_all_threads(void)
{
    int ret = 0;

    struct cfg_node *cnode;
    TAILQ_FOREACH(cnode, &g_configs, node) {
        ret = pthread_join(*(cnode)->thread_data, NULL);
        if (ret) {
            errorf("thread join failed");
            goto bail;
        }
    }

bail:
    return ret;
}

static void *single_mode_run(void *arg)
{
    int ret = -1;
    single_mode_cfg_t *cfg = arg;

    int loop = (cfg->count) ? 0 : 1;
    void *sender_handle = sender_dispatcher[cfg->protocol].worker->create(cfg->ctx);
    if (!sender_handle) {
        errorf("sender creation failed");
        goto bail;
    }

    cfg->sender_handle = sender_handle;
    uint32_t sleep_duration = (cfg->interval_ms) ? (cfg->interval_ms * MSEC) : (100 * USEC);
    while (loop || cfg->count--) {
        infof("[Single Mode] %s", get_localtime());
        sender_dispatcher[cfg->protocol].worker->send(sender_handle, cfg->data, cfg->datalen);
        usleep(sleep_duration);
        fprintf(stderr, "\n");
    }
    sender_dispatcher[cfg->protocol].worker->destroy(sender_handle);
    ret = 0;

bail:
    pthread_exit(&ret);

    return 0;
}

static int prepare_eth_ctx(ethctx_t **ctx, char *ifname, uint8_t *dstmac)
{
    if (!ctx) {
        errorf("[MAIN] eth ctx null");
        return -1;
    }

    *ctx = calloc(1, sizeof(ethctx_t));
    if (!*ctx) {
        errorf("mem alloc failed");
        return -1;
    }

    memcpy((*ctx)->ifname, ifname, IFNAMSIZ - 1);
    memcpy((*ctx)->dstmac, dstmac, ETH_ALEN);
    return 0;
}

static int prepare_udp_ctx(udpctx_t **ctx, char *ip, uint32_t port)
{
    if (!ctx) {
        errorf("[MAIN] udp ctx null");
        return -1;
    }

    *ctx = calloc(1, sizeof(udpctx_t));
    if (!*ctx) {
        errorf("mem alloc failed");
        return -1;
    }

    (*ctx)->port = port;
    strncpy((*ctx)->ip, ip, sizeof((*ctx)->ip) - 1);
    return 0;
}

static int prepare_tcp_ctx(tcpctx_t **ctx, char *ip, uint32_t port)
{
    if (!ctx) {
        errorf("[MAIN] tcp ctx null");
        return -1;
    }

    *ctx = calloc(1, sizeof(tcpctx_t));
    if (!*ctx) {
        errorf("mem alloc failed");
        return -1;
    }

    (*ctx)->port = port;
    strncpy((*ctx)->ip, ip, sizeof((*ctx)->ip) - 1);
    return 0;
}

static void fill_runtime_config(single_mode_cfg_t *cfg, void *ctx, streamtype_e protocol,
    uint32_t interval_ms, uint32_t count, uint8_t *data, uint32_t datalen)
{
    cfg->protocol = protocol;
    cfg->interval_ms = interval_ms;
    cfg->count = count;
    cfg->data = data;
    cfg->datalen = datalen;
    cfg->ctx = ctx;
}

static void cleanup_runtime_config(single_mode_cfg_t *cfg)
{
    if (!cfg) {
        return;
    }

    SFREE(cfg->data);
    SFREE(cfg->ctx);
}

static void destroy_runtime_configs(void)
{
    struct cfg_node *cnode, *tnode;

    TAILQ_FOREACH_SAFE(cnode, &g_configs, node, tnode) {
        cleanup_runtime_config(cnode->cfg);
        SFREE(cnode->thread_data);
        SFREE(cnode->cfg);
        TAILQ_REMOVE(&g_configs, cnode, node);
        SFREE(cnode);
    }
}

int main(int argc, char *argv[])
{
    int c;
    int single_mode = 0;
    char cfg_filename[PATH_MAX] = {0};
    int ret = EXIT_FAILURE;
    int cfg_from_file = 0;
    ethctx_t *ectx = NULL;
    tcpctx_t *tctx = NULL;
    udpctx_t *uctx = NULL;
    unsigned char dstmac[ETH_ALEN] = {0};
    char ifname[IFNAMSIZ] = {0};
    char ip[32] = {0};
    uint32_t port = 0;
    streamtype_e protocol;
    uint8_t *data = NULL;
    uint32_t datalen = 0;
    uint32_t interval_ms = 0;
    uint32_t count = 0;
    pthread_t *thread_data = NULL;
    struct sigaction sigint_action = {.sa_handler = sigint_handler};

    if (argc < 2) {
        print_help();
        goto bail;
    }

    TAILQ_INIT(&g_configs);

    if (sigaction(SIGINT, &sigint_action, NULL)) {
        errorf("[MAIN] Sigint handler set is failed");
        goto bail;
    }

    while (1) {
        int option_index = 0;
        c = getopt_long(argc, argv, "d:i:b:r:c:t:l:of:p:s:a:",
                long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'd':
            if (str2mac(optarg, dstmac)) {
                errorf("MAC format is wrong\n");
                goto bail;
            }
            break;

        case 'i':
            memcpy(ifname, optarg, strlen(optarg));
            break;

        case 'b':
            data = str2hex(optarg, &datalen);
            if (data == NULL) {
                errorf("Data parsing is failed\n");
                goto bail;
            }
            break;

        case 'r':
            errno = 0;
            datalen = strtol(optarg, NULL, 0);
            if (errno) {
                errorf("Failed to parse count\n");
                goto bail;
            }
            data = generate_rand_data((size_t)datalen);
            
            break;

        case 'c':
            errno = 0;
            count = strtol(optarg, NULL, 0);
            if (errno) {
                errorf("Failed to parse count\n");
                goto bail;
            }
            break;

        case 't':
            errno = 0;
            interval_ms = strtol(optarg, NULL, 0);
            if (errno) {
                errorf("Failed to parse count\n");
                goto bail;
            }
            break;

        case 'l':
            set_debug_level(str2debug(optarg));
            break;

        case 'o':
            show_current_opts();
            goto bail;
            break;

        case 'f':
            strncpy(cfg_filename, optarg, sizeof(cfg_filename) - 1);
            cfg_from_file = 1;
            break;

        case 's':
            protocol = get_protocol_from_string(optarg);
            break;

        case 'a':
            strncpy(ip, optarg, sizeof(ip) - 1);
            break;

        case 'p':
            port = atoi(optarg);
            break;

        case '?':
            print_help();
            goto bail;
            break;

        default:
            fprintf(stderr, "?? getopt returned character code 0%o ??\n", c);
        }
    }    

    if (!cfg_from_file) {
        if (data == NULL) {
            errorf("Please, give binary data or random data or a config file");
            goto bail;
        }

        single_mode_cfg_t *cfg = calloc(1, sizeof(*cfg));
        void *current_ctx = NULL;
        if (!cfg) {
            errorf("mem allocation failed");
            goto bail;
        }

        if (protocol == ETH) {
            if (ifname[0] == 0 || dstmac[0] == 0) {
                errorf("Stream is ETH, ifname and destination MAC should be given.");
                goto bail;
            }

            if (prepare_eth_ctx(&ectx, ifname, dstmac)) {
                goto bail;
            }
            current_ctx = ectx;
        } else if (protocol == TCP) {
            if (port == 0 || ip[0] == 0) {
                errorf("Stream is TCP, port and destination IP should be given.");
                goto bail;
            }

            if (prepare_tcp_ctx(&tctx, ip, port)) {
                goto bail;
            }
            current_ctx = tctx;
        } else if (protocol == UDP) {
            if (port == 0 || ip[0] == 0) {
                errorf("Stream is UDP, port and destination IP should be given.");
                goto bail;
            }

            if (prepare_udp_ctx(&uctx, ip, port)) {
                goto bail;
            }
            current_ctx = uctx;
        }

        thread_data = calloc(1, sizeof(*thread_data));
        if (!thread_data) {
            errorf("mem alloc failed");
            goto bail;
        }

        fill_runtime_config(cfg, current_ctx, protocol, interval_ms, count, data, datalen);

        ret = pthread_create(thread_data, NULL, single_mode_run, cfg); 
        if (ret) {
            errorf("thread create failed");
            goto bail;
        }

        struct cfg_node *cnode = calloc(1, sizeof(*cnode));
        if (!cnode) {
            errorf("mem alloc failed");
            ret = -1;
            goto bail;
        }

        cnode->cfg = cfg;
        cnode->thread_data = thread_data;
        TAILQ_INSERT_TAIL(&g_configs, cnode, node);

        ret = pthread_join(*thread_data, NULL);
        if (ret) {
            errorf("thread join failed");
            goto bail;
        }

        goto bail;
    }

    SFREE(data);

    // @todo:
    // capture SIGINT for single mode or other also

    if (config_init(cfg_filename, "json")) {
        errorf("config initialization failed");
        goto bail;
    }

    int tcp_stream_size;
    config_t tcpcfg;
    config_get_stream(&tcpcfg, TCP, &tcp_stream_size);

    for (int i = 0; i < tcpcfg.cfg_size; ++i) {
        stream_config_t *stream = &tcpcfg.streams[i];

        single_mode_cfg_t *cfg = calloc(1, sizeof(*cfg));
        if (!cfg) {
            errorf("mem allocation failed");
            goto bail;
        }

        memset(cfg, 0, sizeof(*cfg));

        cfg->protocol = TCP;
        cfg->ctx = stream->stream_ctx;
        cfg->data = str2hex(stream->data, &cfg->datalen);
        if (!cfg->data) {
            errorf("Data parsing failed");
            goto bail;
        }
        cfg->count = stream->count;
        cfg->interval_ms = stream->interval_ms;

        thread_data = calloc(1, sizeof(*thread_data));
        if (!thread_data) {
            errorf("mem alloc failed");
            goto bail;
        }
        // fill_runtime_config(cfg, cfg->ctx, cfg->protocol, cfg->interval_ms, cfg->count, cfg->data, cfg->datalen);

        ret = pthread_create(thread_data, NULL, single_mode_run, cfg);
        if (ret) {
            errorf("thread create failed");
            goto bail;
        }

        struct cfg_node *cnode = calloc(1, sizeof(*cnode));
        if (!cnode) {
            errorf("mem alloc failed");
            ret = -1;
            goto bail;
        }

        cnode->cfg = cfg;
        cnode->thread_data = thread_data;
        TAILQ_INSERT_TAIL(&g_configs, cnode, node);
    }

    if (join_all_threads()) {
        goto bail;
    }

    // void *ethhandle = sender_dispatcher[ETH].worker->create(&ectx);
    // if (!ethhandle) {
    //     errorf("ETH Handle NULL, exit");
    //     goto bail;
    // }

    // void *udphandle = sender_dispatcher[UDP].worker->create(&uctx);
    // if (!udphandle) {
    //     errorf("UDP Handle NULL, exit");
    //     goto bail;
    // }

    
    

    // while (1)
    // {
    //     ts_begin = gettime_ms();
    //     fprintf(stderr, "ts_begin: %llu\n", ts_begin);
        
    //     fprintf(stderr, "Sending data: ");
    //     dump_bytes_hex(single_cfg.data, single_cfg.datalen);

    //     if (sender_dispatcher[ETH].worker->send(ethhandle, data, datalen)) {
    //         errorf("failed to send raw eth packet");
    //         goto bail;
    //     }

    //     if (sender_dispatcher[UDP].worker->send(udphandle, data, datalen)) {
    //         errorf("SEND UDP FAIL");
    //     }

    //     if (sender_dispatcher[TCP].worker->send(tcphandle, data, datalen)) {
    //         errorf("SEND TCP FAIL");
    //     }

    //     ts_end = gettime_ms();
    //     fprintf(stderr, "ts_end: %llu\n", ts_end);
    //     fprintf(stderr, "delta: %llu ms, cnt: %u\n\n", ts_end - ts_begin, ++cnt);

    //     usleep(interval_in_ms * MSEC);
    // }


    ret = EXIT_SUCCESS;
bail:
    cleanup_main_thread();    
    // sender_dispatcher[ETH].worker->destroy(ethhandle);
    // sender_dispatcher[UDP].worker->destroy(udphandle);
    // sender_dispatcher[TCP].worker->destroy(tcphandle);
    // config_destroy();

    return ret;
}
