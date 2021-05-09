#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <json-c/json.h>
#include <limits.h>

#include <net/if.h>

#include "debug/debug.h"
#include "utils/utils.h"
#include "eth/ethersend.h"
#include "udp/udpsend.h"
#include "tcp/tcpsend.h"
#include "config/config.h"

static void print_help(void);
static void show_current_opts(void);

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
            "-l, --log              set log level"
            "-o, --opt              get current options");
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
    {0,         0,                 0,  0 }
};

struct global_sender {
    char name[16];
    struct sender *worker;
};

struct global_sender sender_dispatcher[TOTAL_SENDER] = {
    {.name = "udp", .worker = &udpsender},
    {.name = "eth", .worker = &ethsender},
    {.name = "tcp", .worker = &tcpsender},
};

int main(int argc, char *argv[])
{
    int c;
    unsigned char dstmac[ETH_ALEN];
    char ifname[IFNAMSIZ] = {0};
    char cfg_filename[PATH_MAX] = {0};
    uint8_t *data = NULL;
    uint32_t datalen = 0;
    long int count;
    long int interval_in_ms;
    int ret = EXIT_FAILURE;

    if (argc < 2) {
        print_help();
        goto bail;
    }

    while (1) {
        int option_index = 0;
        c = getopt_long(argc, argv, "d:i:b:r:c:t:l:of:",
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
            interval_in_ms = strtol(optarg, NULL, 0);
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
            strcpy(cfg_filename, optarg);
            break;

        case '?':
            print_help();
            goto bail;
            break;

        default:
            fprintf(stderr, "?? getopt returned character code 0%o ??\n", c);
        }
    }    
    
    if (data == NULL) {
        errorf("Please, give binary data or random data");
        goto bail;
    }

    long long int ts_begin, ts_end;
    uint32_t cnt = 0;

    if (config_init(cfg_filename, "json")) {
        errorf("config initialization failed");
        goto bail;
    }

    int tcp_stream_size;
    config_t tcpcfg;
    config_get_stream(&tcpcfg, TCP, &tcp_stream_size);

    ethctx_t ectx = {.ptype = 1};
    memcpy(ectx.ifname, ifname, IFNAMSIZ);
    memcpy(ectx.dstmac, dstmac, ETH_ALEN);
    void *ethhandle = sender_dispatcher[ETH].worker->create(&ectx);
    if (!ethhandle) {
        errorf("ETH Handle NULL, exit");
        goto bail;
    }

    udpctx_t uctx = {.port = 11221};
    snprintf(uctx.ip, MAX_IP_STR_SIZE, "192.168.2.1");
    void *udphandle = sender_dispatcher[UDP].worker->create(&uctx);
    if (!udphandle) {
        errorf("UDP Handle NULL, exit");
        goto bail;
    }

    tcpctx_t tctx = {.port = 3333};
    snprintf(tctx.ip, MAX_IP_STR_SIZE, "0.0.0.0");
    void *tcphandle = sender_dispatcher[TCP].worker->create(&tctx);
    if (!tcphandle) {
        errorf("TCP Handle NULL, exit");
        goto bail;
    }

    while (1)
    {
        ts_begin = gettime_ms();
        fprintf(stderr, "ts_begin: %llu\n", ts_begin);
        
        fprintf(stderr, "Sending data: ");
        dump_bytes_hex(data, datalen);

        if (sender_dispatcher[ETH].worker->send(ethhandle, data, datalen)) {
            errorf("failed to send raw eth packet");
            goto bail;
        }

        if (sender_dispatcher[UDP].worker->send(udphandle, data, datalen)) {
            errorf("SEND UDP FAIL");
        }

        if (sender_dispatcher[TCP].worker->send(tcphandle, data, datalen)) {
            errorf("SEND TCP FAIL");
        }

        ts_end = gettime_ms();
        fprintf(stderr, "ts_end: %llu\n", ts_end);
        fprintf(stderr, "delta: %llu ms, cnt: %u\n\n", ts_end - ts_begin, ++cnt);

        usleep(interval_in_ms * MSEC);
    }


    ret = EXIT_SUCCESS;
bail:
    SFREE(data);
    sender_dispatcher[ETH].worker->destroy(ethhandle);
    sender_dispatcher[UDP].worker->destroy(udphandle);
    sender_dispatcher[TCP].worker->destroy(tcphandle);
    config_destroy();

    return ret;
}
