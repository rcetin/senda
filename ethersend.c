#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <linux/if_packet.h>

#include "utils/utils.h"
#include "debug/debug.h"
#include "ethersend.h"
#include "main.h"

void eth_create(void)
{

}

static int eth_open(void)
{
    int sockfd;
    /* Open RAW socket to send on */
    sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
    if (sockfd == -1) {
        errorf("socket failed. errno: %d, strerr: %s", errno, strerror(errno));
    }
    return sockfd;
}

int eth_send(void *ctx, uint8_t *data, uint32_t len)
{
    int sockfd;
	struct ifreq if_idx;
	struct ifreq if_mac;
	uint64_t tx_len = 0;
	char sendbuf[MAX_BUFFER_SIZE];
	struct ether_header *eh = (struct ether_header *) sendbuf;
	struct iphdr *iph = (struct iphdr *) (sendbuf + sizeof(struct ether_header));
	struct sockaddr_ll socket_address;
    struct ethctx *ectx = (struct ethctx *)ctx;
    int ret = -1;

    debugf("Send raw eth packet to: "ETHER_STR", from %s", ETHER_ADDR(ectx->dstmac), ectx->ifname);
    printf("Data: ");
    for (uint32_t i = 0; i < len; ++i) {
        printf("0x%02x ", data[i]);
    }
    printf("\n");

	sockfd = eth_open();
    if (sockfd < 0) {
        errorf("socket open failed");
        return -1;
    }

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
    printmac(eh->ether_shost);
    printmac(eh->ether_dhost);

	/* Ethertype field */
	eh->ether_type = htons(packet2proto(ectx->ptype));
	tx_len += sizeof(struct ether_header);

	/* Packet data */
    memcpy(sendbuf + tx_len, data, len);
    tx_len += len;

    memset(&socket_address, 0, sizeof(socket_address));
    socket_address.sll_family = AF_PACKET;
	/* Index of the network device */
	socket_address.sll_ifindex = if_idx.ifr_ifindex;
	/* Address length*/
	socket_address.sll_halen = ETH_ALEN;
	/* Destination MAC */
    memcpy(socket_address.sll_addr, ectx->dstmac, ETH_ALEN);
    printmac(socket_address.sll_addr);

	/* Send packet */
    uint32_t cnt = 0;
    ret = (int)sendto(sockfd, sendbuf, tx_len, MSG_CONFIRM, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));
    if (ret == -1) {
        errorf("sendto error. errno: %d, strerr: %s", errno, strerror(errno));
        goto bail;
    }

    debugf("sendto succeed. Send %u bytes to "ETHER_STR" from: %s", ret, ETHER_ADDR(ectx->dstmac), ectx->ifname);

    ret = 0;
bail:
    close(sockfd);
    return ret;
}

struct sender ethsender = {
    .create = eth_create,
    .send = eth_send,
};