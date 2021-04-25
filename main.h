#ifndef _MAIN_H
#define _MAIN_H

#define TOTAL_SENDER 2

typedef void(*fptr_create)(void);
typedef int(*fptr_send)(void *ctx, uint8_t *data, uint32_t len);

struct sender {
    fptr_create create;
    fptr_send send;
};

#endif