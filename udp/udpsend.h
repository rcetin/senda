#ifndef _UDP_SEND
#define _UDP_SEND

#include "main.h"
#include "utils/utils.h"

struct sender udpsender;

typedef struct udpaddr {
    char ip[MAX_IP_STR_SIZE];
    int port;
} udpaddr_t;

void *udp_create(void *ctx);
int udp_send(void *priv, uint8_t *data, uint32_t len);
void udp_destroy(void *priv);

#endif /* _UDP_SEND */