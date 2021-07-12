#ifndef _ETHERSEND_H
#define _ETHERSEND_H

#include <net/if.h>

struct sender ethsender;

typedef struct ethctx {
    char ifname[IFNAMSIZ];
    uint8_t dstmac[ETH_ALEN];
    packet_t ptype;
} ethctx_t;

void *eth_create(void *ctx);
int eth_send(void *priv, const uint8_t *data, uint32_t len);
void eth_destroy(void *priv);

#endif