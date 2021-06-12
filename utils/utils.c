#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>
#include <sys/ioctl.h>

#include <netinet/ether.h>
#include <net/if.h>

#include "utils.h"
#include "../debug/debug.h"

int str2mac(const char* str, uint8_t* mac)
{
    struct ether_addr addr;
    if (ether_aton_r(str, &addr) == NULL) {
        errorf("failed to convert str to mac for: %s", str);
        return -1;
    }

    memcpy(mac, addr.ether_addr_octet, ETH_ALEN);
    return 0;
}

int mac2str(const uint8_t* mac, char *buf)
{
    struct ether_addr addr;
    memcpy(addr.ether_addr_octet, mac, ETH_ALEN);

    if (ether_aton_r(buf, &addr) == NULL) {
        errorf("failed to convert mac to str for: "ETHER_STR, ETHER_ADDR(mac));
        return -1;
    }

    return 0;
}

uint8_t *str2hex(char *str, uint32_t *datalen)
{
    uint32_t len;
    uint8_t *elem;
    unsigned int tmp;

    if (str == NULL) {
        return NULL;
    }

    len = (strlen(str) % 2) ? (strlen(str) / 2) + 1 : (strlen(str) / 2);
    if (len <= 0) {
        return NULL;
    }

    errorf("calloc len: %d", len);
    elem = (uint8_t *)calloc(1, len);
    if (elem == NULL) {
        errorf("failed to allocate\n");
        return NULL;
    }

    errorf("Data: %s", str);
    for (uint32_t i = 0; i < len; ++i, str += 2) {
        sscanf(str, "%02x", &tmp);
        elem[i] = tmp;
        errorf("%p->%d:%02x", (void *)&elem[i], i, elem[i]);
    }
        
    *datalen = len;

    return elem;
}

void printmac(uint8_t *mac)
{
    fprintf(stderr, "MAC: "ETHER_STR"\n", ETHER_ADDR(mac));
}

long long int gettime_ms(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts)) {
        errorf("failed to get time. errno: %d, strerr: %s", errno, strerror(errno));
        return -1;
    }

    debugf("clock_gettime succeed. ts.sec: %ld, ts.nsec: %ld", ts.tv_sec, ts.tv_nsec);

    return ts.tv_sec * MSEC + ts.tv_nsec / USEC;
}

const char *get_localtime(void)
{
    time_t rawtime;
    struct tm * timeinfo;
    char *buf;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    buf = asctime(timeinfo);

    buf[strlen(buf) - 1] = 0;
    return buf;
}

uint8_t *generate_rand_data(size_t size)
{
    static int seeded = 0;

    if (!seeded) {
        srand(time(NULL));
        seeded = 1;
    }

    uint8_t *data = malloc(size);
    if (!data) {
        errorf("failed to allocate data for random");
        return NULL;
    }

    for(size_t i = 0; i < size; ++i) {
        data[i] = rand() % UINT8_MAX;
        fprintf(stderr, "0x%2x ", data[i]);
    }

    infof("\nrandom data generated size: %lu", size);
    return data;
}

unsigned int packet2proto(packet_t p)
{
    if (p == ARP) return ETH_P_ARP;
    else return ETH_P_IP;
}


int get_ifidx(int sockfd, struct ifreq *ifidx, const char *ifname)
{
    int ret;

    /* Get the index of the interface to send on */
	memset(ifidx, 0, sizeof(struct ifreq));
	strncpy(ifidx->ifr_name, ifname, IFNAMSIZ - 1);
    ret = ioctl(sockfd, SIOCGIFINDEX, ifidx);
	if (ret == -1) {
        errorf("ioctl SIOCGIFINDEX error. errno: %d, strerr: %s", errno, strerror(errno));
    }

    return ret;
}

int get_ifmac(int sockfd, struct ifreq *ifmac, const char *ifname)
{
    int ret;

    /* Get the MAC address of the interface to send on */
	memset(ifmac, 0, sizeof(struct ifreq));
	strncpy(ifmac->ifr_name, ifname, IFNAMSIZ - 1);
    ret = ioctl(sockfd, SIOCGIFHWADDR, ifmac);
	if (ret == -1) {
        errorf("ioctl SIOCGIFHWADDR error. errno: %d, strerr: %s", errno, strerror(errno));
    }

    return ret;
}

void dump_bytes_hex(uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; ++i) {
        fprintf(stderr, "0x%02x ", data[i]);
    }
    fprintf(stderr, "\n");
}