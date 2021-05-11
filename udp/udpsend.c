#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "utils/utils.h"
#include "debug/debug.h"
#include "udpsend.h"
#include "main.h"

typedef struct udp_priv {
    int handle;
    udpctx_t addr;
} udp_priv_t;

static int udp_open(void)
{
    return socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
}

void *udp_create(void *ctx)
{
    udpctx_t *addr = ctx;
    udp_priv_t *private = NULL;
    
    int fd = udp_open();
    if (fd == -1) {
        errorf("[UDP] socket open error");
        return NULL;
    }

    private = calloc(1, sizeof(*private));
    if (!private) {
        errorf("[UDP] no memory for handle");
        return NULL;
    }

    private->handle = fd;
    memcpy(private->addr.ip, addr->ip, sizeof(private->addr.ip));
    private->addr.port = addr->port;

    infof("[UDP] handle created successfully. fd: %u", fd);

    return private;
}

int udp_send(void *priv, uint8_t *data, uint32_t len)
{
    udp_priv_t *private = priv;
    int sockfd = private->handle;
    struct sockaddr_in si;
    int ret = 0;
    udpctx_t *udpctx = &private->addr;

    debugf("[UDP] ENTER");

    memset((char *) &si, 0, sizeof(si));
	si.sin_family = AF_INET;
	si.sin_port = htons(udpctx->port);
	
    if (inet_aton(udpctx->ip, &si.sin_addr) == 0) {
        errorf("[UDP] invalid ip addr: %s", udpctx->ip);
        goto bail;
    }

    ret = sendto(sockfd, data, len, 0, (struct sockaddr *) &si, sizeof(si));
    if (ret == -1) {
        errorf("[UDP] sendto error. errno: %d, strerr: %s", errno, strerror(errno));
        goto bail;
    }

    debugf("[UDP] send success. Send %u bytes", ret);
    ret = 0;
bail:
    errorf("[UDP] Returning ret: %d", ret);
    return ret;
}

void udp_destroy(void *priv)
{
    udp_priv_t *private = priv;

    if (!private) {
        return;
    }

    close(private->handle);
    SFREE(priv);
}

struct sender udpsender = {
    .create = udp_create,
    .send = udp_send,
    .destroy = udp_destroy,
};
