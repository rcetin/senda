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
            "-s, --sender           sender type (\"tcp\", \"udp\", \"eth\")\n"
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
    unsigned char dstmac[ETH_ALEN];
    char ifname[IFNAMSIZ];
    char ip[32];
    uint32_t port;
    uint8_t *data;
    uint32_t datalen;
    uint32_t interval_ms;
    uint32_t count;
    void *ctx;
} single_mode_cfg_t;

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

static void *single_mode_run(void *arg)
{
    single_mode_cfg_t *cfg = arg;
    int loop = (cfg->count) ? 0 : 1;
    void *sender_handle = sender_dispatcher[cfg->protocol].worker->create(cfg->ctx);
    uint32_t sleep_duration = (cfg->interval_ms) ? (cfg->interval_ms * MSEC) : (100 * USEC);
    while (loop || cfg->count--) {
        infof("[Single Mode] %s", get_localtime());
        sender_dispatcher[cfg->protocol].worker->send(sender_handle, cfg->data, cfg->datalen);
        usleep(sleep_duration);
        fprintf(stderr, "\n");
    }
    sender_dispatcher[cfg->protocol].worker->destroy(sender_handle);

    pthread_exit(0);

    return 0;
}

int main(int argc, char *argv[])
{
    int c;
    int single_mode = 0;
    char cfg_filename[PATH_MAX] = {0};
    int ret = EXIT_FAILURE;
    int cfg_from_file = 0;
    ethctx_t ectx = {.ptype = 1};
    tcpctx_t tctx = {0};
    udpctx_t uctx = {0};
    single_mode_cfg_t single_cfg;

    if (argc < 2) {
        print_help();
        goto bail;
    }

    memset(&single_cfg, 0, sizeof(single_cfg));

    while (1) {
        int option_index = 0;
        c = getopt_long(argc, argv, "d:i:b:r:c:t:l:of:p:s:a:",
                long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'd':
            if (str2mac(optarg, single_cfg.dstmac)) {
                errorf("MAC format is wrong\n");
                goto bail;
            }
            break;

        case 'i':
            memcpy(single_cfg.ifname, optarg, strlen(optarg));
            break;

        case 'b':
            single_cfg.data = str2hex(optarg, &single_cfg.datalen);
            if (single_cfg.data == NULL) {
                errorf("Data parsing is failed\n");
                goto bail;
            }
            break;

        case 'r':
            errno = 0;
            single_cfg.datalen = strtol(optarg, NULL, 0);
            if (errno) {
                errorf("Failed to parse count\n");
                goto bail;
            }
            single_cfg.data = generate_rand_data((size_t)single_cfg.datalen);
            
            break;

        case 'c':
            errno = 0;
            single_cfg.count = strtol(optarg, NULL, 0);
            if (errno) {
                errorf("Failed to parse count\n");
                goto bail;
            }
            break;

        case 't':
            errno = 0;
            single_cfg.interval_ms = strtol(optarg, NULL, 0);
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
            strncpy(cfg_filename, optarg, sizeof(cfg_filename - 1));
            cfg_from_file = 1;
            break;

        case 's':
            single_cfg.protocol = get_protocol_from_string(optarg);
            break;

        case 'a':
            strncpy(single_cfg.ip, optarg, sizeof(single_cfg.ip) - 1);
            fprintf(stderr, "ip given: %s, sizeof(single_cfg.ip): %lu, copied: %s\n\n", optarg, sizeof(single_cfg.ip), single_cfg.ip);
            break;

        case 'p':
            single_cfg.port = atoi(optarg);
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
        if (single_cfg.data == NULL) {
            errorf("Please, give binary data or random data or a config file");
            goto bail;
        }

        // Single mode
        single_mode = 1;
        if (single_cfg.protocol == ETH) {
            if (single_cfg.ifname[0] == 0 || single_cfg.dstmac[0] == 0) {
                errorf("Stream is ETH, ifname and destination MAC should be given.");
                goto bail;
            }
            memcpy(ectx.ifname, single_cfg.ifname, IFNAMSIZ);
            memcpy(ectx.dstmac, single_cfg.dstmac, ETH_ALEN);
            single_cfg.ctx = &ectx;
        } else if (single_cfg.protocol == TCP) {
            if (single_cfg.port == 0 || single_cfg.ip[0] == 0) {
                errorf("Stream is TCP, port and destination IP should be given.");
                goto bail;
            }
            tctx.port = single_cfg.port;
            strncpy(tctx.ip, single_cfg.ip, sizeof(tctx.ip) - 1);
            single_cfg.ctx = &tctx;
        } else if (single_cfg.protocol == UDP) {
            if (single_cfg.port == 0 || single_cfg.ip[0] == 0) {
                errorf("Stream is UDP, port and destination IP should be given.");
                goto bail;
            }
            uctx.port = single_cfg.port;
            strncpy(uctx.ip, single_cfg.ip, sizeof(uctx.ip) - 1);
            single_cfg.ctx = &uctx;
        }

        pthread_t sthread_id;
        int *retval = NULL;
        ret = pthread_create(&sthread_id, NULL, single_mode_run, &single_cfg); 
        if (ret) {
            errorf("thread create failed");
            goto bail;
        }

        ret = pthread_join(sthread_id, (void **)&retval);
        if (ret) {
            errorf("thread join failed");
            goto bail;
        }

        goto bail;
    }

    // @todo:
    // capture SIGINT for single mode or other also

    // long long int ts_begin, ts_end;
    // uint32_t cnt = 0;

    // if (config_init(cfg_filename, "json")) {
    //     errorf("config initialization failed");
    //     goto bail;
    // }

    // int tcp_stream_size;
    // config_t tcpcfg;
    // config_get_stream(&tcpcfg, TCP, &tcp_stream_size);

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

    
    
    // void *tcphandle = sender_dispatcher[TCP].worker->create(&tctx);
    // if (!tcphandle) {
    //     errorf("TCP Handle NULL, exit");
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
    if (single_mode) {
        SFREE(single_cfg.data);
    }
    
    // sender_dispatcher[ETH].worker->destroy(ethhandle);
    // sender_dispatcher[UDP].worker->destroy(udphandle);
    // sender_dispatcher[TCP].worker->destroy(tcphandle);
    // config_destroy();

    return ret;
}
