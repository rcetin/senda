#ifndef _TCPSEND_H
#define _TCPSEND_H

#include <stdint.h>

typedef struct tcpaddr {
    char ip[16];
    int port;
} tcpaddr_t;

struct sender tcpsender;

void *tcp_create(void *ctx);
int tcp_send(void *priv, uint8_t *data, uint32_t len);
void tcp_destroy(void *priv);

#endif
