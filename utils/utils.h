#ifndef _UTILS_H
#define _UTILS_H

#include <sys/queue.h>
#include <stdlib.h>

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

#ifndef CHECK_BIT
#define CHECK_BIT(val, mask)    !!(val & mask)
#endif

#ifndef SET_BIT
#define SET_BIT(val, mask)    (val |= mask)
#endif

#ifndef RESET_BIT
#define RESET_BIT(val, mask)    (val &= ~mask)
#endif

#ifndef TAILQ_END
#define	TAILQ_END(head)			(NULL)
#endif

#ifndef TAILQ_FOREACH_SAFE
#define	TAILQ_FOREACH_SAFE(var, head, field, next)			\
	for ((var) = ((head)->tqh_first);				\
	    (var) != TAILQ_END(head) &&					\
	    ((next) = TAILQ_NEXT(var, field), 1); (var) = (next))
#endif

#define BASE_16 16
#define MAX_DATA_SIZE       (1*1024*1024)
#define MAX_BUFFER_SIZE     1024
#define MAX_IP_STR_SIZE     16
#define SEC 1
#define MSEC (1000 * SEC)
#define USEC (1000 * MSEC)
#define INTERVAL_MS_DEFAULT 1000
#define COUNT_DEFAULT       0

typedef enum {
    ARP,
} packet_t;

struct ifreq;

uint8_t *str2hex(char*, uint32_t*);
int str2mac(const char*, uint8_t*);
int mac2str(const uint8_t*, char*);
unsigned int packet2proto(packet_t);
long long int gettime_ms(void);
const char *get_localtime(void);
uint8_t *generate_rand_data(size_t);

void printmac(uint8_t *mac);
void dump_bytes_hex(uint8_t *data, uint32_t len);

int get_ifidx(int sockfd, struct ifreq *ifidx, const char *ifname);
int get_ifmac(int sockfd, struct ifreq *ifmac, const char *ifname);

#endif

