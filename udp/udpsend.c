#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "utils/utils.h"
#include "debug/debug.h"
#include "udpsend.h"
#include "main.h"

typedef struct udp_priv {
    int handle;
    udpaddr addr;
} udp_priv;

static int udp_open(void)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == -1) {
        errorf("udp socket open error");
    }

    return sockfd;
}

void *udp_create(void *ctx)
{
    udpaddr *addr = ctx;
    udp_priv *private = NULL;
    
    int fd = udp_open();
    if (fd == -1) {
        return NULL;
    }

    private = calloc(1, sizeof(*private));
    if (!private) {
        errorf("no memory for udp handle");
        return NULL;
    }

    errorf("sock success. fd: %u", fd);
    private->handle = fd;
    memcpy(private->addr.ip, addr->ip, sizeof(private->addr.ip));
    private->addr.port = addr->port;

    return private;
}

int udp_send(void *priv, uint8_t *data, uint32_t len)
{
    udp_priv *private = priv;
    int sockfd = private->handle;
    struct sockaddr_in si;
    int ret = 0;
    udpaddr *udpa = &private->addr;

    errorf("ENTER");

    memset((char *) &si, 0, sizeof(si));
	si.sin_family = AF_INET;
	si.sin_port = htons(udpa->port);
	
    if (inet_aton(udpa->ip, &si.sin_addr) == 0) {
        errorf("invalid ip addr");
        goto bail;
    }

    ret = sendto(sockfd, data, len, 0, (struct sockaddr *) &si, sizeof(si));
    if (ret == -1) {
        errorf("sendto error. errno: %d, strerr: %s", errno, strerror(errno));
        goto bail;
    }

    ret = 0;
    errorf("UDP send success ret: %d", ret);
bail:
    errorf("UDP send exiting ret: %d", ret);
    return ret;
}

void udp_destroy(void *priv)
{
    udp_priv *private = priv;
    close(private->handle);
    SFREE(priv);
}

struct sender udpsender = {
    .create = udp_create,
    .send = udp_send,
    .destroy = udp_destroy,
};
