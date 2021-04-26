#ifndef _TCPSEND_H
#define _TCPSEND_H

#include <stdint.h>

struct tcpaddr {
    char ip[16];
    int port;
};

struct sender tcpsender;

int tcp_create(void);
int tcp_send(int handle, void *ctx, uint8_t *data, uint32_t len);
void tcp_destroy(int handle);

#endif
