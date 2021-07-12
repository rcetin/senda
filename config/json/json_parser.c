#include <json-c/json.h>
#include <stdio.h>
#include <string.h>

#include "json_parser.h"
#include "config/config.h"
#include "debug/debug.h"
#include "tcp/tcpsend.h"
#include "utils/utils.h"
#include "eth/ethersend.h"

static void dump_cfg(stream_config_t *cfg)
{
    fprintf(stderr, "data: %s\n", cfg->data);
    fprintf(stderr, "count: %u\n", cfg->count);
    fprintf(stderr, "interval_ms: %u\n", cfg->interval_ms);
}

static int json_get_tcp_udp_info(json_object *object, void *outctx)
{
    transportctx_t *ctx = NULL;

    if (!outctx) {
        return -1;
    }

    ctx = outctx;
    
    const char *ip = json_object_get_string(json_object_object_get(object, "ip"));
    if (!ip) {
        errorf("[JSON] get stream IP failed");
        return -1;
    }

    const char *port = json_object_get_string(json_object_object_get(object, "port"));
    if (!port) {
        errorf("[JSON] get stream port failed");
        return -1;
    }

    memcpy(ctx->ip, ip, strlen(ip));
    ctx->port = atoi(port);

    return 0;
}

static const char *read_data_from_file(const char *filename)
{
    int alloc_size = 1024;
    int remaining = alloc_size;
    int idx = 0;
    size_t read;

    char *buffer = calloc(alloc_size, sizeof(char));
    if (!buffer) {
        perror("[JSON] alloc failed");
        return NULL;
    }

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("[JSON] open failed");
        return NULL;
    }

    do {
        read = fread(buffer + idx, 1, remaining, fp);
        if (!read) {
            // return buffer;
            fprintf(stderr, "Read file completed.\n\n");
            break;
        }

        remaining -= read;
        idx += read;
        if (!remaining) {
            char *new_buf = realloc(buffer, 2 * alloc_size);
            if (!new_buf) {
                perror("[JSON] realloc failed");
                free(buffer);
                return NULL;
            }

            remaining += alloc_size;
            alloc_size *= 2;
            fprintf(stderr, "enlarge the buffer, new size=%d\n", alloc_size);
            buffer = new_buf;
        }
    } while(1);

    return buffer;
    
}

static int json_get_eth_info(json_object *object, void *outctx)
{
    ethctx_t *ctx = NULL;

    if (!outctx) {
        return -1;
    }

    ctx = outctx;
    
    const char *ifname = json_object_get_string(json_object_object_get(object, "ifname"));
    if (!ifname) {
        errorf("[JSON] get ifname failed");
        return -1;
    }

    const char *dstmac = json_object_get_string(json_object_object_get(object, "dstmac"));
    if (!dstmac) {
        errorf("[JSON] get dstmac failed");
        return -1;
    }

    strncpy(ctx->ifname, ifname, sizeof(ctx->ifname) - 1);
    str2mac(dstmac, ctx->dstmac);
    ctx->ptype = ARP; // @TODO: more packet types dynamically

    return 0;
}

static int json_get_stream(const char *filename, streamtype_e stream_type, config_t *cfg, int *stream_size)
{
    int ret = -1;
    FILE *fp = NULL;
    char buffer[2048] = {0};
    int streams_len = 0;
    int proto_streams_len = 0;
    struct {
        json_object *core;
        json_object *streams;
        json_object *stream_elem;
        json_object *proto;
        json_object *proto_elem;
    } json = {NULL, NULL, NULL, NULL, NULL};
    char stream_names[MAX_STREAM_TYPE][4] = {{"udp"}, {"eth"}, {"tcp"}};
    void *stream_contexes[MAX_STREAM_TYPE] = {0};
    size_t ctx_sizes[MAX_STREAM_TYPE] = {sizeof(transportctx_t), sizeof(ethctx_t), sizeof(transportctx_t)};
    int (*get_ctx_fn[MAX_STREAM_TYPE])(json_object *, void *) = {json_get_tcp_udp_info, json_get_eth_info, json_get_tcp_udp_info};

    debugf("[JSON] Enter");
    infof("[JSON] Reading config for %s stream.", stream_names[stream_type]);

    if (!stream_size) {
        errorf("[JSON] null stream_size");
        goto bail;
    }

    memset(cfg, 0, sizeof(*cfg));

    fp = fopen(filename, "r");
    if (!fp) {
        errorf("[JSON] file open error: %s", filename);
        goto bail;
    }

    if (!fread(buffer, 1, sizeof(buffer), fp)) {
        errorf("[JSON] file read error");
        goto bail;
    }

    json.core = json_tokener_parse(buffer);
    if (!json.core) {
        errorf("[JSON] parsing error");
        goto bail;
    }

    json.streams = json_object_object_get(json.core, "streams");
    if (!json.streams) {
        errorf("[JSON] parsing error");
        goto bail;
    }

    streams_len = json_object_array_length(json.streams);
    errorf("[%s] streams len = %u", stream_names[stream_type], streams_len);

    for (int i = 0; i < streams_len; ++i) {
        
        json.stream_elem = json_object_array_get_idx(json.streams, (size_t)i);

        json.proto = json_object_object_get(json.stream_elem, stream_names[stream_type]);
        if (!json.proto) {
            errorf("[JSON] proto field cannot be found");
            continue;
        }

        cfg->cfg_size = json_object_array_length(json.proto);
        errorf("proto: %s, stram len = %u", stream_names[stream_type], cfg->cfg_size);

        if (!cfg->cfg_size) {
            cfg->streams = NULL;
            goto bail;
        }

        cfg->streams = calloc(1, cfg->cfg_size * sizeof(stream_config_t));
        if (!cfg->streams) {
            errorf("[JSON] mem allocation failed");
            goto bail;
        }

        for (int j = 0; j < cfg->cfg_size; ++j) {
            json.proto_elem = json_object_array_get_idx(json.proto, (size_t)j);

            cfg->streams[j].stream_type = stream_type;
            const char *data = json_object_get_string(json_object_object_get(json.proto_elem, "data"));
            if (!data) {
                errorf("[JSON] get stream data failed");
                goto bail;
            }
            if (!strncmp(data, "/", 1)) {
                // read from path
                data = read_data_from_file(data);
            } else {
                cfg->streams[j].convertDataToHex = 1;
            }

            cfg->streams[j].data = calloc(strlen(data) + 1, sizeof(char));
            if (!cfg->streams[j].data) {
                errorf("[JSON] alloc failed");
                goto bail;
            }
            memcpy(cfg->streams[j].data, data, strlen(data));

            stream_contexes[stream_type] = calloc(1, ctx_sizes[stream_type]);
            if (!stream_contexes[stream_type]) {
                errorf("[JSON] mem allocation failed");
                goto bail;
            }

            get_ctx_fn[stream_type](json.proto_elem, stream_contexes[stream_type]);

            const char *count = json_object_get_string(json_object_object_get(json.proto_elem, "count"));
            cfg->streams[j].count = (!count) ? COUNT_DEFAULT : atoi(count);

            const char *interval_ms = json_object_get_string(json_object_object_get(json.proto_elem, "interval_ms"));
            cfg->streams[j].interval_ms = (!interval_ms) ? INTERVAL_MS_DEFAULT: atoi(interval_ms);

            cfg->streams[j].stream_ctx = stream_contexes[stream_type];
            // dump_cfg(&cfg->streams[j]);
        }
    }

    // dump file 
    fprintf(stderr, "\n\n%s\n", buffer);

    ret = 0;
bail:
    if (json.core) { json_object_put(json.core); }
    if (fp) { fclose(fp); }
    if (ret) {
            SFREE(cfg->streams);
            cfg->cfg_size = 0;
    }

    debugf("[JSON] Returning ret: %d", ret);
    return ret;
}

config_worker_t json_worker = {
    .get_stream = json_get_stream,
};