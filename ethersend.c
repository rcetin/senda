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

int send_raw_eth(const char *ifname, uint8_t *dstmac, uint8_t *data, uint32_t len)
{
    int sockfd;
	struct ifreq if_idx;
	struct ifreq if_mac;
	uint64_t tx_len = 0;
	char sendbuf[MAX_BUFFER_SIZE];
	struct ether_header *eh = (struct ether_header *) sendbuf;
	struct iphdr *iph = (struct iphdr *) (sendbuf + sizeof(struct ether_header));
	struct sockaddr_ll socket_address;
    int ret = -1;

    debugf("Send raw eth packet to: "ETHER_STR", from %s", ETHER_ADDR(dstmac), ifname);
    printf("Data: ");
    for (uint32_t i = 0; i < len; ++i) {
        printf("0x%02x ", data[i]);
    }
    printf("\n");

	/* Open RAW socket to send on */
    sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
    if (sockfd == -1) {
        errorf("socket failed. errno: %d, strerr: %s", errno, strerror(errno));
        return ret;
    }

	/* Get the index of the interface to send on */
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, ifname, IFNAMSIZ - 1);
    ret = ioctl(sockfd, SIOCGIFINDEX, &if_idx);
	if (ret == -1) {
        errorf("ioctl SIOCGIFINDEX error. errno: %d, strerr: %s", errno, strerror(errno));
        goto bail;
    }

	/* Get the MAC address of the interface to send on */
	memset(&if_mac, 0, sizeof(struct ifreq));
	strncpy(if_mac.ifr_name, ifname, IFNAMSIZ - 1);
    ret = ioctl(sockfd, SIOCGIFHWADDR, &if_mac);
	if (ret == -1) {
        errorf("ioctl SIOCGIFHWADDR error. errno: %d, strerr: %s", errno, strerror(errno));
        goto bail;
    }

	/* Construct the Ethernet header */
	memset(sendbuf, 0, MAX_BUFFER_SIZE);

	/* Ethernet header */
    memcpy(eh->ether_shost, if_mac.ifr_hwaddr.sa_data, ETH_ALEN);
    memcpy(eh->ether_dhost, dstmac, ETH_ALEN);
    printmac(eh->ether_shost);
    printmac(eh->ether_dhost);

	/* Ethertype field */
	eh->ether_type = htons(ETH_P_ARP);
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
    memcpy(socket_address.sll_addr, dstmac, ETH_ALEN);
    printmac(socket_address.sll_addr);

	/* Send packet */
    uint32_t cnt = 0;
    ret = (int)sendto(sockfd, sendbuf, tx_len, MSG_CONFIRM, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));
    if (ret == -1) {
        errorf("sendto error. errno: %d, strerr: %s", errno, strerror(errno));
        goto bail;
    }

    debugf("sendto succeed. Send %u bytes to "ETHER_STR" from: %s", ret, ETHER_ADDR(dstmac), ifname);

    ret = 0;
bail:
    close(sockfd);
    return ret;
}
