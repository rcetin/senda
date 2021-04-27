#ifndef _MAIN_H
#define _MAIN_H

typedef void*(*fptr_create)(void *ctx);
typedef int(*fptr_send)(void *priv, uint8_t *data, uint32_t len);
typedef void(*fptr_destroy)(void *priv);

struct sender {
    fptr_create create;
    fptr_send send;
    fptr_destroy destroy;
};

#endif