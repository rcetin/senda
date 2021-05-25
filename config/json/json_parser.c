#include <json-c/json.h>
#include <stdio.h>
#include <string.h>

#include "json_parser.h"
#include "config/config.h"
#include "debug/debug.h"
#include "tcp/tcpsend.h"
#include "utils/utils.h"

static int json_get_tcp_stream(const char *filename, config_t *cfg, int *stream_size)
{
    int ret = -1;
    FILE *fp = NULL;
    char buffer[2048] = {0};
    int streams_len = 0;
    int tcp_streams_len = 0;
    struct {
        json_object *core;
        json_object *streams;
        json_object *stream_elem;
        json_object *tcp;
        json_object *tcp_elem;
    } json = {NULL, NULL, NULL, NULL, NULL};

    debugf("[JSON] Enter");

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
    errorf("[TCP] streams len = %u", streams_len);

    for (int i = 0; i < streams_len; ++i) {
        
        json.stream_elem = json_object_array_get_idx(json.streams, (size_t)i);

        json.tcp = json_object_object_get(json.stream_elem, "tcp");
        if (!json.tcp) {
            errorf("[JSON] tcp field cannot be found");
            continue;
        }

        cfg->cfg_size = json_object_array_length(json.tcp);
        errorf("[TCP] tcp stram len = %u", cfg->cfg_size);

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
            json.tcp_elem = json_object_array_get_idx(json.tcp, (size_t)j);



            cfg->streams[j].stream_type = TCP;
            const char *data = json_object_get_string(json_object_object_get(json.tcp_elem, "data"));
            if (!data) {
                errorf("[JSON] get stream data failed");
                goto bail;
            }
            memcpy(cfg->streams[j].data, data, strlen(data));

            tcpctx_t *tcpctx = calloc(1, sizeof(tcpctx_t));
            if (!tcpctx) {
                errorf("[JSON] mem allocation failed");
                goto bail;
            }

            const char *ip = json_object_get_string(json_object_object_get(json.tcp_elem, "ip"));
            if (!ip) {
                errorf("[JSON] get stream IP failed");
                goto bail;
            }
            infof("iplen: %lu, ip=%s\n", strlen(ip), ip);
            memcpy(tcpctx->ip, ip, strlen(ip));
            infof("tcpctx->ipiplen: %lu, tcpctx->ipip=%s\n", strlen(tcpctx->ip), tcpctx->ip);

            const char *port = json_object_get_string(json_object_object_get(json.tcp_elem, "port"));
            if (!port) {
                errorf("[JSON] get stream port failed");
                goto bail;
            }
            tcpctx->port = atoi(port);

            const char *count = json_object_get_string(json_object_object_get(json.tcp_elem, "count"));
            cfg->streams[j].count = (!count) ? COUNT_DEFAULT : atoi(count);

            const char *interval_ms = json_object_get_string(json_object_object_get(json.tcp_elem, "interval_ms"));
            cfg->streams[j].interval_ms = (!interval_ms) ? INTERVAL_MS_DEFAULT: atoi(interval_ms);

            cfg->streams[j].stream_ctx = tcpctx;
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
    .get_tcp_stream = json_get_tcp_stream,
};