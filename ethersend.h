#ifndef _ETHERSEND_H
#define _ETHERSEND_H

int send_raw_eth(const char *ifname, uint8_t *dstmac, uint8_t *data, uint32_t len);


#endif