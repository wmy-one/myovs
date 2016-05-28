#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "udp.h"

int udp_port_init(struct udp *udp, const char *server_ip)
{
	int sfd;

	sfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sfd == -1) {
		printf("failed at socket()\n");
		return sfd;
	}

	int opt = SO_REUSEADDR;
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	bzero(&udp->si, sizeof(udp->si));
	udp->si.sin_family = AF_INET;
	udp->si.sin_port = htons(udp->port);

	if ((unsigned long int)server_ip != INADDR_ANY) {
		if (inet_aton(server_ip, &udp->si.sin_addr) == 0) {
			printf("failed at inet_aton()\n");
			close(sfd);
			return -1;
		}
	} else {
		udp->si.sin_addr.s_addr = htonl(INADDR_ANY);

		if (bind(sfd, (const struct sockaddr *)&udp->si, sizeof(udp->si)) == -1) {
			printf("failed at bind()\n");
			close(sfd);
			return -1;
		}
	}

	udp->sfd = sfd;

	opt = udp->buf_size * 8;
	if (setsockopt(sfd, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt)) < 0)
		printf("failed to set sock SO_RCVBUF\n");
	if (setsockopt(sfd, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt)) < 0)
		printf("failed to set sock SO_SNDBUF\n");

	return 0;
}

struct udp *udp_ports_init(const char *server_ip, unsigned int start_port,
						   unsigned int port_num, unsigned int buf_size)
{
	struct udp *udp;
	int ret;
	int i;

	if (start_port + port_num > 65535 || buf_size == 0)
		return NULL;

	udp = (struct udp *)malloc(port_num * sizeof(*udp));
	if (udp == NULL) {
		printf("failed at udp malloc()\n");
		return NULL;
	}

	for (i = 0; i < port_num; i++) {
		udp[i].port = start_port + i;
		udp[i].buf_size = buf_size;
		ret = udp_port_init(&udp[i], server_ip);
		if (ret < 0)
			goto udp_port_init_failed;
	}

	return udp;

udp_port_init_failed:
	while (i >= 0)
		close(udp[--i].sfd);

	return NULL;
}

static inline int setnonblocking(int fd)
{
	int old_opt = fcntl(fd, F_GETFL);
	int new_opt = old_opt | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_opt);
	return old_opt;
}

int udp_epoll_init(struct udp *udp, unsigned int len, int event)
{
	struct epoll_event ev;
	int epfd;
	int ret;

	if (!udp || len == 0 || (event != EPOLLIN && event != EPOLLOUT))
			return -1;

	epfd = epoll_create(len);
	if (epfd <= 0) {
		printf("failed at epoll_create()\n");
		return -1;
	}

	for (int i = 0; i < len; i++) {
		ev.data.ptr = &udp[i];
		ev.events = event | EPOLLET;
		ret = epoll_ctl(epfd, EPOLL_CTL_ADD, udp[i].sfd, &ev);
		if (ret < 0) {
			printf("failed at EPOLL_CTL_ADD\n");
			close(epfd);
			return -1;
		}
		setnonblocking(udp[i].sfd);
	}

	return epfd;
}

inline void udp_ports_deinit(struct udp *udp, unsigned int len)
{
	if (!udp || len == 0)
		return;

	for (int j = 0; j < len; j++)
		close(udp[j].sfd);
	free(udp);
}
