#ifndef _TCPSEND_H
#define _TCPSEND_H

#include <stdint.h>

#include "utils/utils.h"

struct sender tcpsender;

void *tcp_create(void *ctx);
int tcp_send(void *priv, const uint8_t *data, uint32_t len);
void tcp_destroy(void *priv);

#endif
