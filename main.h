#ifndef _MAIN_H
#define _MAIN_H

typedef int(*fptr_create)(void *ctx);
typedef int(*fptr_send)(int handle, uint8_t *data, uint32_t len);
typedef void(*fptr_destroy)(int handle);

struct sender {
    fptr_create create;
    fptr_send send;
    fptr_destroy destroy;
};

#endif