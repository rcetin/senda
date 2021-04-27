#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include "tcpsend.h"
#include "main.h"
#include "debug/debug.h"
#include "utils/utils.h"

typedef struct tcp_priv {
    int handle;
    tcpaddr_t addr;
} tcp_priv_t;

static int tcp_open(void)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sockfd == -1) {
        errorf("tcp socket open error");
    }

    return sockfd;
}

static int tcp_connect(tcp_priv_t *priv)
{
    struct sockaddr_in si;
    tcpaddr_t *tcpa = &priv->addr;
    int ret = 0;

    memset((char *) &si, 0, sizeof(si));
	si.sin_family = AF_INET;
	si.sin_port = htons(tcpa->port);
	
    printf("TCP1\n");
    if (inet_aton(tcpa->ip, &si.sin_addr) == 0) {
        errorf("invalid ip addr");
        ret = -1;
        goto bail;
    }

    if (connect(priv->handle , (struct sockaddr *)&si , sizeof(si)) < 0)
	{
		errorf("TCP connect error");
        ret = -1;
		goto bail;
	}

bail:
    return ret;
}

void *tcp_create(void *ctx)
{
    tcpaddr_t *addr = ctx;
    tcp_priv_t *private = NULL;

    int fd = tcp_open();
    if (fd == -1) {
        return NULL;
    }

    private = calloc(1, sizeof(*private));
    if (!private) {
        errorf("no memory for tcp handle");
        return NULL;
    }

    errorf("sock success. fd: %u", fd);
    private->handle = fd;
    memcpy(private->addr.ip, addr->ip, sizeof(private->addr.ip));
    private->addr.port = addr->port;

    // Set a signal handler for SIGPIPE
    

    if (tcp_connect(private)) {
        goto bail;
    }

    return private;

bail:
    SFREE(private);
    return NULL;
}

int tcp_send(void *priv, uint8_t *data, uint32_t len)
{
    tcp_priv_t *private = priv;
    int sockfd = private->handle;
    struct sockaddr_in si;
    int ret = 0;
    struct tcpaddr *tcpa = &private->addr;

    errorf("ENTER");

    memset((char *) &si, 0, sizeof(si));
	si.sin_family = AF_INET;
	si.sin_port = htons(tcpa->port);

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

void tcp_destroy(void *priv)
{
    tcp_priv_t *private = priv;
    close(private->handle);
    SFREE(priv);
}

struct sender tcpsender = {
    .create = tcp_create,
    .send = tcp_send,
    .destroy = tcp_destroy,
};
