#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "tcpsend.h"
#include "main.h"
#include "debug/debug.h"

static int tcp_open(void)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sockfd == -1) {
        errorf("tcp socket open error");
    }

    return sockfd;
}

int tcp_connect()
{
    struct sockaddr_in si;

    memset((char *) &si, 0, sizeof(si));
	si.sin_family = AF_INET;
	si.sin_port = htons(tcpa->port);
	
    printf("TCP1\n");
    if (inet_aton(tcpa->ip, &si.sin_addr) == 0) {
        errorf("invalid ip addr");
        ret = -1;
        goto bail;
    }

    if (connect(sockfd , (struct sockaddr *)&si , sizeof(si)) < 0)
	{
		errorf("TCP connect error");
        ret = -1;
		goto bail;
	}
}

int tcp_create(void)
{
    int fd = tcp_open();
    if (fd == -1) {
        return -1;
    }

    errorf("sock success. fd: %u", fd);
    return fd;

}

int tcp_send(int handle, void *ctx, uint8_t *data, uint32_t len)
{
    int sockfd = handle;
    struct sockaddr_in si;
    int ret = 0;
    struct tcpaddr *tcpa = (struct tcpaddr *)ctx;

    errorf("ENTER");

    memset((char *) &si, 0, sizeof(si));
	si.sin_family = AF_INET;
	si.sin_port = htons(tcpa->port);
	
    printf("TCP1\n");
    if (inet_aton(tcpa->ip, &si.sin_addr) == 0) {
        errorf("invalid ip addr");
        ret = -1;
        goto bail;
    }

    if (connect(sockfd , (struct sockaddr *)&si , sizeof(si)) < 0)
	{
		errorf("TCP connect error");
        ret = -1;
		goto bail;
	}

    printf("TCP2\n");
    ret = sendto(sockfd, data, len, 0, (struct sockaddr *) &si, sizeof(si));
    if (ret == -1) {
        errorf("sendto error. errno: %d, strerr: %s", errno, strerror(errno));
        goto bail;
    }

    printf("TCP3\n");
    ret = 0;
    errorf("TCP send success ret: %d", ret);
bail:
    errorf("TCP send exiting ret: %d", ret);
    return ret;
}

void tcp_destroy(int handle)
{
    int sockfd = handle;
    close(sockfd);
}


struct sender tcpsender = {
    .create = tcp_create,
    .send = tcp_send,
    .destroy = tcp_destroy,
};
