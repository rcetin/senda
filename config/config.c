#include <string.h>
#include <limits.h>

#include "config.h"
#include "main.h"
#include "debug/debug.h"
#include "json/json_parser.h"

typedef struct config {
    char filename[PATH_MAX];
    configtype_e configtype;
    config_worker_t *worker;
} config_t;

config_t g_config;

int config_init(const char *filename, const char *configtype)
{
    debugf("[CONFIG] Enter");

    strcpy(g_config.filename, filename);
    if (!strcmp(configtype, "json")) {
        g_config.configtype = JSON;
        g_config.worker = &json_worker;

    } else {
        errorf("[CONFIG] wrong config type!");
        return -1;
    }

    infof("[CONFIG] New config is created.");
    debugf("[CONFIG] returning");
    return 0;
}

int config_get_stream(void **ctx, sendertype_e stream_type, int *stream_size)
{
    switch (stream_type)
    {
    case TCP:
        g_config.worker->get_tcp_stream(ctx, stream_size);
        break;
    default:
        break;
    }
    return 0;
}