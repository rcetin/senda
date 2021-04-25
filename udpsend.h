#ifndef _UDP_SEND
#define _UDP_SEND

#include "main.h"

struct sender udpsender;

struct udpaddr {
    char ip[16];
    int port;
};

void udp_create(void);
int udp_send(void *ctx, uint8_t *data, uint32_t len);


#endif /* _UDP_SEND */