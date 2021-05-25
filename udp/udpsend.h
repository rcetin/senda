#ifndef _UDP_SEND
#define _UDP_SEND

#include "main.h"
#include "utils/utils.h"

struct sender udpsender;

void *udp_create(void *ctx);
int udp_send(void *priv, uint8_t *data, uint32_t len);
void udp_destroy(void *priv);

#endif /* _UDP_SEND */