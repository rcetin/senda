#ifndef _UTILS_H
#define _UTILS_H

#ifndef ETH_ALEN
#define ETH_ALEN    6
#endif

#ifndef ETHER_STR
#define ETHER_STR   "%x:%x:%x:%x:%x:%x"
#endif

#ifndef ETHER_ADDR
#define ETHER_ADDR(addr)    addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]
#endif

#define BASE_16 16
#define MAX_BUFFER_SIZE   1024
#define SEC 1
#define MSEC (1000 * SEC)
#define USEC (1000 * MSEC)

uint8_t *str2hex(char *str, uint32_t *datalen);
int str2mac(const char* str, uint8_t* mac);
int mac2str(const uint8_t* mac, char *buf);
long long int gettime_ms(void);

void printmac(uint8_t *mac);

#endif

