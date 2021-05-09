#ifndef _CONFIG_H
#define _CONFIG_H

#include "main.h"
#include "utils/utils.h"

typedef enum configtype {
    JSON,
    CONFIG_ALL,
} configtype_e;

typedef struct stream_config {
    char data[MAX_DATA_SIZE];
    streamtype_e stream_type;
    void *stream;   // tcpctx_t, ethctx_t, udpctx_t
} stream_config_t;

typedef struct config {
    int cfg_size;
    stream_config_t *streams;
} config_t;

typedef struct config_worker {
    int (*get_tcp_stream)(const char *filename, config_t *cfg_out, int *stream_size);
} config_worker_t;

int config_init(const char *filename, const char *configtype);
int config_get_stream(config_t *cfg_out, streamtype_e stream_type, int *stream_size);
void config_destroy(void);

#endif