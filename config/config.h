#ifndef _CONFIG_H
#define _CONFIG_H

#include "main.h"

typedef enum configtype {
    JSON,
    CONFIG_ALL,
} configtype_e;

typedef int (*fptr_get_tcp_stream)(void **ctx, int *stream_size);

typedef struct config_worker {
    fptr_get_tcp_stream get_tcp_stream;
} config_worker_t;

int config_init(const char *filename, const char *configtype);
int config_get_stream(void **ctx, sendertype_e stream_type, int *stream_size);

#endif