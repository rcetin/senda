#include <json-c/json.h>

#include "json_parser.h"
#include "config/config.h"
#include "debug/debug.h"

static int json_get_tcp_stream(void **ctx, int *stream_size)
{
    int ret = 0;
    debugf("[JSON] Enter");

    infof("TEST json tcp stream");

    debugf("[JSON] Returning ret: %d", ret);
    return ret;
}

config_worker_t json_worker = {
    .get_tcp_stream = json_get_tcp_stream,
};