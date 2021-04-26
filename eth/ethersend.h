#ifndef _ETHERSEND_H
#define _ETHERSEND_H

struct sender ethsender;

struct ethctx {
    const char *ifname;
    uint8_t *dstmac;
    packet_t ptype;
};

int eth_create(void);
int eth_send(int handle, void *ctx, uint8_t *data, uint32_t len);
void eth_destroy(int handle);

#endif