#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/select.h>

#include "tcpsend.h"
#include "main.h"
#include "debug/debug.h"
#include "utils/utils.h"

#include "tcppriv.h"

typedef struct tcp_priv {
    int handle;
    uint32_t flags;
    transportctx_t addr;
} tcp_priv_t;

struct sigaction sigpipe_action = {.sa_handler = SIG_IGN};

static int tcp_open(void)
{
    return socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_IP);
}

static int tcp_connect(int fd, char *ip, uint32_t port)
{
    struct sockaddr_in si;
    int ret = 0;
    int arg = 0;

    memset((char *) &si, 0, sizeof(si));
	si.sin_family = AF_INET;
	si.sin_port = htons(port);
	
    if (inet_aton(ip, &si.sin_addr) == 0) {
        errorf("[TCP] invalid ip addr: %s", ip);
        ret = -1;
        goto bail;
    }

    arg = fcntl(fd, F_GETFL, NULL);
    if (arg < 0) {
        errorf("[TCP] GETFL failed");
        ret = -1;
        goto bail;
    }

    if (connect(fd , (struct sockaddr *)&si , sizeof(si)) < 0) {
        if (errno == EINPROGRESS) {
            struct timeval tv;
            fd_set tcpwrset;

            tv.tv_sec = 15;
            tv.tv_usec = 0;
            FD_ZERO(&tcpwrset);
            FD_SET(fd, &tcpwrset);


            ret = select(fd + 1, NULL, &tcpwrset, NULL, &tv);
            if (ret < 0) {
                errorf("[TCP] select failed");
                goto bail;
            }

            if (!ret) {
                errno = 0;
                errorf("[TCP] Connection timeout");
                ret = -1;
                goto bail;
            }

            if (ret == 1) {
                if (FD_ISSET(fd, &tcpwrset)) {
                    int so_error;
                    socklen_t len = sizeof(so_error);
                    ret = 0;

                    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &len)) {
                        errorf("[TCP] getsockopt failed");
                        ret = -1;
                        goto bail;
                    }

                    if (so_error) {
                        errno = so_error;
                        errorf("[TCP] Connection failed");
                        ret = -1;
                        goto bail;
                    }

                    arg &= ~O_NONBLOCK;
                    if (fcntl(fd, F_SETFL, arg)) {
                        errorf("[TCP] SETFL failed");
                        ret = -1;
                        goto bail;
                    }

                    infof("[TCP] Connection successful");
                }
            }
        }
	}

bail:
    return ret;
}

void *tcp_create(void *ctx)
{
    transportctx_t *addr = ctx;
    tcp_priv_t *private = NULL;

    fprintf(stderr, "TCP ctx: %p\n", ctx);

    int fd = tcp_open();
    if (fd == -1) {
        errorf("[TCP] socket open error");
        return NULL;
    }

    // Set a signal handler for SIGPIPE
    if (sigaction(SIGPIPE, &sigpipe_action, NULL)) {
        errorf("[TCP] Sigpipe handler set is failed");
        goto bail;
    }

    if (tcp_connect(fd, addr->ip, addr->port)) {
        goto bail;
    }

    private = calloc(1, sizeof(*private));
    if (!private) {
        errorf("[TCP] no memory for tcp handle");
        return NULL;
    }

    private->handle = fd;
    strncpy(private->addr.ip, addr->ip, sizeof(private->addr.ip) - 1);
    private->addr.port = addr->port;

    infof("[TCP] handle created successfully. fd: %u, ip: %s, port: %u", fd, private->addr.ip, private->addr.port);
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
    ret = tcp_connect(private->handle, private->addr.ip, private->addr.port);
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
    transportctx_t *tcpx = &private->addr;

    debugf("[TCP] ENTER");

    if (CHECK_BIT(private->flags, TCP_FLAGS_BROKEN_PIPE) &&
        tcp_handle_brokenpipe(private)) {
        goto bail;
    }

    memset((char *) &si, 0, sizeof(si));
	si.sin_family = AF_INET;
	si.sin_port = htons(tcpx->port);

    ret = write(sockfd, data, len);
    // ret = sendto(sockfd, data, len, 0, (struct sockaddr *) &si, sizeof(si));
    if (ret == -1) {
        if (errno == EPIPE && !CHECK_BIT(private->flags, TCP_FLAGS_BROKEN_PIPE)) {
            debugf("[TCP] Setting TCP Broken Pipe flag");
            SET_BIT(private->flags, TCP_FLAGS_BROKEN_PIPE);
        }
        errorf("[TCP] Sendto error. errno: %d, strerr: %s", errno, strerror(errno));
        goto bail;
    }

    infof("[TCP] send success. [%u] bytes, [%s]->[%s]", ret, "0.0.0.0", tcpx->ip);
    ret = 0;
bail:
    debugf("[TCP] Returning ret: %d", ret);
    return ret;
}

void tcp_destroy(void *priv)
{
    tcp_priv_t *private = priv;
    if (!private) {
        errorf("[TCP] private null");
        return;
    }

    infof("[TCP] destroy: %u", private->handle);
    close(private->handle);
    SFREE(priv);
    priv = NULL;
}

struct sender tcpsender = {
    .create = tcp_create,
    .send = tcp_send,
    .destroy = tcp_destroy,
};
