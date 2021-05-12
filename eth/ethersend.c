#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <linux/if_packet.h>

#include "utils/utils.h"
#include "debug/debug.h"
#include "ethersend.h"
#include "main.h"

typedef struct eth_priv {
    int handle;
    ethctx_t ctx;
} eth_priv;

static int eth_open(void)
{
    return socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
}

void *eth_create(void *ctx)
{
    ethctx_t *ectx = ctx;
    eth_priv *private = NULL;

	int sockfd = eth_open();
    if (sockfd < 0) {
        errorf("[ETH] socket open failed");
        return NULL;
    }

    private = calloc(1, sizeof(*private));
    if (!private) {
        errorf("[ETH] no memory for handle");
        return NULL;
    }

    private->handle = sockfd;
    memcpy(private->ctx.ifname, ectx->ifname, IFNAMSIZ);
    memcpy(private->ctx.dstmac, ectx->dstmac, ETH_ALEN);
    private->ctx.ptype = ectx->ptype;

    return private;
}

int eth_send(void *priv, uint8_t *data, uint32_t len)
{
    eth_priv *private = priv;
    int sockfd = private->handle;
	struct ifreq if_idx;
	struct ifreq if_mac;
	uint64_t tx_len = 0;
	char sendbuf[MAX_BUFFER_SIZE];
	struct ether_header *eh = (struct ether_header *) sendbuf;
	struct iphdr *iph = (struct iphdr *) (sendbuf + sizeof(struct ether_header));
	struct sockaddr_ll socket_address;
    struct ethctx *ectx = &private->ctx;
    int ret = -1;

    debugf("[ETH] ENTER");

    ret = get_ifidx(sockfd, &if_idx, ectx->ifname);
    if (ret) {
        goto bail;
    }

    ret = get_ifmac(sockfd, &if_mac, ectx->ifname);
    if (ret) {
        goto bail;
    }

	/* Construct the Ethernet header */
	memset(sendbuf, 0, MAX_BUFFER_SIZE);

	/* Ethernet header */
    memcpy(eh->ether_shost, if_mac.ifr_hwaddr.sa_data, ETH_ALEN);
    memcpy(eh->ether_dhost, ectx->dstmac, ETH_ALEN);

	/* Ethertype field */
	eh->ether_type = htons(packet2proto(ectx->ptype));
	tx_len += sizeof(struct ether_header);

	/* Packet data */
    memcpy(sendbuf + tx_len, data, len);
    tx_len += len;

    memset(&socket_address, 0, sizeof(socket_address));
    socket_address.sll_family = AF_PACKET;
	socket_address.sll_ifindex = if_idx.ifr_ifindex;
	socket_address.sll_halen = ETH_ALEN;
    memcpy(socket_address.sll_addr, ectx->dstmac, ETH_ALEN);

	/* Send packet */
    uint32_t cnt = 0;
    ret = (int)sendto(sockfd, sendbuf, tx_len, MSG_CONFIRM, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));
    if (ret == -1) {
        errorf("[ETH] sendto error. errno: %d, strerr: %s", errno, strerror(errno));
        goto bail;
    }

    infof("[ETH] send success. [%u] bytes, ["ETHER_STR"]->["ETHER_STR"]", ret, ETHER_ADDR(if_mac.ifr_hwaddr.sa_data), ETHER_ADDR(ectx->dstmac));
    ret = 0;
bail:
    debugf("[ETH] Returning, ret: %d", ret);
    return ret;
}

void eth_destroy(void *priv)
{
    eth_priv *private = priv;

    if (!private) {
        return;
    }

    close(private->handle);
    SFREE(priv);
    priv = NULL;
}

struct sender ethsender = {
    .create = eth_create,
    .send = eth_send,
    .destroy = eth_destroy,
};