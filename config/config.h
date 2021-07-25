#ifndef _CONFIG_H
#define _CONFIG_H

#include "main.h"
#include "utils/utils.h"

typedef enum configtype {
    JSON,
    CONFIG_ALL,
} configtype_e;

typedef struct stream_config {
    uint8_t *data;
    size_t datalen;
    uint8_t mapped;
    streamtype_e stream_type;
    uint32_t count;
    uint32_t interval_ms;
    void *stream_ctx;   // tcpctx_t, ethctx_t, udpctx_t
} stream_config_t;

typedef struct config {
    int cfg_size;
    stream_config_t *streams;
} config_t;

typedef struct config_worker {
    int (*get_stream)(const char *filename, streamtype_e stream_type, config_t *cfg_out, int *stream_size);
} config_worker_t;

int config_init(const char *filename, const char *configtype);
int config_get_stream(config_t *cfg_out, streamtype_e stream_type, int *stream_size);
void config_destroy(void);

#endif