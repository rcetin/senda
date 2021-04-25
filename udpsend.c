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

void udp_create(void)
{
    errorf("udp handle is created");
}

static int udp_open(void)
{
    int sockfd;

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == -1) {
        errorf("udp socket open error");
    }

    return sockfd;
}

int udp_send(void *ctx, uint8_t *data, uint32_t len)
{
    int sockfd;
    struct sockaddr_in si;
    int ret = 0;
    struct udpaddr *udpa = (struct udpaddr *)ctx;

    errorf("ENTER");

    sockfd = udp_open();
    if (sockfd == -1) {
        return -1;
    }

    errorf("sock success. fd: %u", sockfd);

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
    printf("UDP send success ret: %d", ret);
bail:
    close(sockfd);
    return ret;
}

struct sender udpsender = {
    .create = udp_create,
    .send = udp_send,
};
