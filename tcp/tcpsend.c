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

#include "tcppriv.h"

typedef struct tcp_priv {
    int handle;
    uint32_t flags;
    tcpctx_t addr;
} tcp_priv_t;

struct sigaction sigpipe_action = {.sa_handler = SIG_IGN};

static int tcp_open(void)
{
    return socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
}

static int tcp_connect(tcp_priv_t *priv)
{
    struct sockaddr_in si;
    tcpctx_t *tcpx = &priv->addr;
    int ret = 0;

    memset((char *) &si, 0, sizeof(si));
	si.sin_family = AF_INET;
	si.sin_port = htons(tcpx->port);
	
    if (inet_aton(tcpx->ip, &si.sin_addr) == 0) {
        errorf("[TCP] invalid ip addr");
        ret = -1;
        goto bail;
    }

    if (connect(priv->handle , (struct sockaddr *)&si , sizeof(si)) < 0) {
		errorf("[TCP] connect error");
        ret = -1;
		goto bail;
	}

bail:
    return ret;
}

void *tcp_create(void *ctx)
{
    tcpctx_t *addr = ctx;
    tcp_priv_t *private = NULL;

    int fd = tcp_open();
    if (fd == -1) {
        errorf("[TCP] socket open error");
        return NULL;
    }

    private = calloc(1, sizeof(*private));
    if (!private) {
        errorf("[TCP] no memory for tcp handle");
        return NULL;
    }

    private->handle = fd;
    memcpy(private->addr.ip, addr->ip, sizeof(private->addr.ip));
    private->addr.port = addr->port;

    // Set a signal handler for SIGPIPE
    if (sigaction(SIGPIPE, &sigpipe_action, NULL)) {
        errorf("[TCP] Sigpipe handler set is failed");
        goto bail;
    }

    if (tcp_connect(private)) {
        goto bail;
    }

    infof("[TCP] handle created successfully. fd: %u", fd);
    return private;
bail:
    SFREE(private);
    return NULL;
}

static int tcp_handle_brokenpipe(void *priv)
{
    tcp_priv_t *private = priv;
    int ret;

    close(private->handle);

    int fd = tcp_open();
    if (fd == -1) {
        errorf("[TCP][Broken Pipe] TCP socket open is failed again.");
        return -1;
    }

    private->handle = fd;
    ret = tcp_connect(private);
    if (ret) {
        errorf("[TCP][Broken Pipe] Reconnection is failed.");
        return -1;
    }

    infof("[TCP] Reconnection is successful. Starting to TCP send again.");
    RESET_BIT(private->flags, TCP_FLAGS_BROKEN_PIPE);
    return 0;
}

int tcp_send(void *priv, uint8_t *data, uint32_t len)
{
    tcp_priv_t *private = priv;
    int sockfd = private->handle;
    struct sockaddr_in si;
    int ret = 0;
    struct tcpctx *tcpx = &private->addr;

    debugf("[TCP] ENTER");

    if (CHECK_BIT(private->flags, TCP_FLAGS_BROKEN_PIPE) &&
        tcp_handle_brokenpipe(private)) {
        goto bail;
    }

    memset((char *) &si, 0, sizeof(si));
	si.sin_family = AF_INET;
	si.sin_port = htons(tcpx->port);

    ret = sendto(sockfd, data, len, 0, (struct sockaddr *) &si, sizeof(si));
    if (ret == -1) {
        if (errno == EPIPE && !CHECK_BIT(private->flags, TCP_FLAGS_BROKEN_PIPE)) {
            debugf("[TCP] Setting TCP Broken Pipe flag");
            SET_BIT(private->flags, TCP_FLAGS_BROKEN_PIPE);
        }
        errorf("[TCP] Sendto error. errno: %d, strerr: %s", errno, strerror(errno));
        goto bail;
    }

    debugf("[TCP] send success. Send %u bytes", ret);
    ret = 0;
bail:
    debugf("[TCP] Returning ret: %d", ret);
    return ret;
}

void tcp_destroy(void *priv)
{
    tcp_priv_t *private = priv;
    if (!private) {
        return;
    }

    close(private->handle);
    SFREE(priv);
}

struct sender tcpsender = {
    .create = tcp_create,
    .send = tcp_send,
    .destroy = tcp_destroy,
};
