#include <json-c/json.h>
#include <stdio.h>
#include <string.h>

#include "json_parser.h"
#include "config/config.h"
#include "debug/debug.h"
#include "tcp/tcpsend.h"

static int json_get_tcp_stream(const char *filename, config_t *cfg, int *stream_size)
{
    int ret = -1;
    FILE *fp = NULL;
    char buffer[2048] = {0};
    int stream_len;
    struct {
        json_object *core;
        json_object *streams;
        json_object *stream_elem;
    } json = {NULL, NULL, NULL};

    debugf("[JSON] Enter");

    if (!stream_size) {
        errorf("[JSON] null stream_size");
        goto bail;
    }

    memset(cfg, 0, sizeof(*cfg));

    fp = fopen(filename, "r");
    if (!fp) {
        errorf("[JSON] file open error");
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

    json.streams = json_object_object_get(json.core, "stream");
    if (!json.streams) {
        errorf("[JSON] parsing error");
        goto bail;
    }

    stream_len = json_object_array_length(json.streams);

    for (int i = 0; i < stream_len; ++i) {
        json.stream_elem = json_object_array_get_idx(json.streams, (size_t)i);
        const char *type = json_object_get_string(json_object_object_get(json.stream_elem, "type"));
        if (strncmp(type, "tcp", 3)) {
            continue;
        }

        cfg->cfg_size++;

        void *cfg_ptr = realloc(cfg->streams, cfg->cfg_size * sizeof(stream_config_t));
        if (!cfg_ptr) {
            errorf("[JSON] mem allocation failed");
            goto bail;
        }
        cfg->streams = cfg_ptr;

        cfg->streams->stream_type = TCP;
        const char *data = json_object_get_string(json_object_object_get(json.stream_elem, "data"));
        if (!data) {
            errorf("[JSON] get stream data failed");
            goto bail;
        }
        memcpy(cfg->streams->data, data, strlen(data));

        tcpctx_t *tcpctx = calloc(1, sizeof(tcpctx_t));
        if (!tcpctx) {
            errorf("[JSON] mem allocation failed");
            goto bail;
        }
        memset(tcpctx, 0, sizeof(tcpctx_t));
        const char *ip = json_object_get_string(json_object_object_get(json.stream_elem, "ip"));
        if (!ip) {
            errorf("[JSON] get stream IP failed");
            goto bail;
        }
        fprintf(stderr, "iplen: %lu, ip=%s\n", strlen(ip), ip);
        memcpy(tcpctx->ip, ip, strlen(ip));

        const char *port = json_object_get_string(json_object_object_get(json.stream_elem, "port"));
        if (!port) {
            errorf("[JSON] get stream port failed");
            goto bail;
        }
        tcpctx->port = atoi(port);

        cfg->streams->stream_ctx = tcpctx;
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