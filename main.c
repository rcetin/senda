#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>

#include <net/if.h>

#include "debug/debug.h"
#include "utils/utils.h"
#include "ethersend.h"

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

int main(int argc, char *argv[])
{
    int c;
    unsigned char dstmac[ETH_ALEN];
    char ifname[IFNAMSIZ] = {0};
    uint8_t *data = NULL;
    uint32_t datalen = 0;
    long int count;
    long int interval_in_ms;

    if (argc < 2) {
        print_help();
        goto bail;
    }

    while (1) {
        int option_index = 0;
        c = getopt_long(argc, argv, "d:i:b:r:c:t:l:o",
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
    while (1)
    {
        ts_begin = gettime_ms();
        fprintf(stderr, "ts_begin: %llu\n", ts_begin);
        if (send_raw_eth(ifname, dstmac, data, datalen, 1)) {
            errorf("failed to send raw eth packet");
            goto bail;
        }
        ts_end = gettime_ms();
        fprintf(stderr, "ts_end: %llu\n", ts_end);
        fprintf(stderr, "delta: %llu ms, cnt: %u\n\n", ts_end - ts_begin, ++cnt);

        sleep(1);
    }
    

    return 0;

bail:
    SFREE(data);
    return EXIT_FAILURE;
}
