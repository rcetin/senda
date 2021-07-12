#ifndef _MAIN_H
#define _MAIN_H

#include <stdint.h>

typedef void*(*fptr_create)(void *ctx);
typedef int(*fptr_send)(void *priv, const uint8_t *data, uint32_t len);
typedef void(*fptr_destroy)(void *priv);

typedef enum streamtype_e {
    UDP,
    ETH,
    TCP,
    MAX_STREAM_TYPE,
} streamtype_e;

struct sender {
    fptr_create create;
    fptr_send send;
    fptr_destroy destroy;
};

#endif