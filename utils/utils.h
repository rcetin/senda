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

#ifndef SFREE
#define SFREE(p)    do{ if (p != NULL) {free(p); p = NULL;} }while(0);
#endif

#define BASE_16 16
#define MAX_BUFFER_SIZE   1024
#define MAX_IP_STR_SIZE   16
#define SEC 1
#define MSEC (1000 * SEC)
#define USEC (1000 * MSEC)

typedef enum {
    ARP,
} packet_t;

struct ifreq;

uint8_t *str2hex(char*, uint32_t*);
int str2mac(const char*, uint8_t*);
int mac2str(const uint8_t*, char*);
unsigned int packet2proto(packet_t);
long long int gettime_ms(void);
uint8_t *generate_rand_data(size_t);

void printmac(uint8_t *mac);

int get_ifidx(int sockfd, struct ifreq *ifidx, const char *ifname);
int get_ifmac(int sockfd, struct ifreq *ifmac, const char *ifname);

#endif

