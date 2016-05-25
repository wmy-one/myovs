#include "cs_com.h"

struct udpserver {
	int port;
	int sfd;
};

int udp_port_init(int port)
{
	struct sockaddr_in si_me;
	int sfd;

	sfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sfd == -1) {
		printf("[ERROR]: failed at socket()\n");
		return sfd;
	}

	int opt = SO_REUSEADDR;
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	bzero(&si_me, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);

	si_me.sin_port = htons(port);
	if (bind(sfd, (const struct sockaddr *)&si_me, sizeof(si_me)) == -1) {
		printf("[ERROR]: failed at bind()\n");
		close(sfd);
		return -1;
	}

	return sfd;
}

struct udpserver *udp_ports_init(int start_port, int port_num)
{
	struct udpserver *server;
	int i;

	server = (struct udpserver *)malloc(port_num * sizeof(*server));
	if (server == NULL) {
		printf("[ERROR]: failed at server malloc()\n");
		return NULL;
	}

	for (i = 0; i < port_num; i++) {
		server[i].sfd = udp_port_init(start_port + i);
		server[i].port = start_port + i;
		if (server[i].sfd < 0)
			goto udp_port_init_failed;
	}

	return server;

udp_port_init_failed:
	while (i >= 0)
		close(server[--i].sfd);

	return NULL;
}

int udpin_epoll_init(struct udpserver *server, int len)
{
	struct epoll_event ev;
	int epfd;
	int ret;

	epfd = epoll_create(len);
	if (epfd <= 0) {
		printf("[ERROR]: failed at epoll_create()\n");
		return -1;
	}

	for (int i = 0; i < len; i++) {
		ev.data.ptr = &server[i];
		ev.events = EPOLLIN | EPOLLET;
		ret = epoll_ctl(epfd, EPOLL_CTL_ADD, server[i].sfd, &ev);
		if (ret < 0) {
			printf("[ERROR]: failed at EPOLL_CTL_ADD\n");
			close(epfd);
			return -1;
		}
		setnonblocking(server[i].sfd);
	}

	return epfd;
}

int main(int argc, char *argv[])
{
	struct sockaddr_in si_other;
	int slen = sizeof(si_other);
	struct udpserver *server;
	int server_cnt = 0;
	char buf[BUFLEN];
//	int ret;
	int i;

	if (argc != 3) {
		perror("ERROR: ./xxx [Start Port] [Ports Number]");
		return 1;
	}

	int start_port = atoi(argv[1]);
	int port_num = atoi(argv[2]);
	ERROR_CHECK(start_port + port_num <= 65535, direct_out);

	server = udp_ports_init(start_port, port_num);
	ERROR_CHECK(server != NULL, direct_out);

	int epfd = udpin_epoll_init(server, port_num);
	ERROR_CHECK(epfd > 0, direct_out);

	struct epoll_event *events;
	events = (struct epoll_event *)malloc(sizeof(*events) * port_num);
	ERROR_CHECK(events != NULL, epoll_add_failed);

	while (1) {
		int nfds = epoll_wait(epfd, events, port_num, -1);
		for (i = 0; i < nfds; i++) {
			if (events[i].events & EPOLLIN) {
				struct udpserver *svr = (struct udpserver *)events[i].data.ptr;
				if (recvfrom(svr->sfd, buf, BUFLEN, 0, (struct sockaddr *)&si_other,
							 (socklen_t *)&slen) == -1) {
					printf("[ERROR]: udp[%d] port[%d] receive failed\n", svr->sfd, svr->port);
				} else {
					// Check buffer data
					for (i = 0; i < BUFLEN; i++) {
						if (buf[i] != TEST_DATA) {
							printf("[ERROR]: udp[%d] port[%d] data check failed\n", svr->sfd, svr->port);
							goto epoll_add_failed;
						}
					}
					printf("[DBG]: udp[%d] port[%d] data check success\n", svr->sfd, svr->port);
				}
			}
		}
	}

	close(epfd);

	for (int j = 0; j < server_cnt; j++)
		close(server[j].sfd);
	free(server);

	return 0;

epoll_add_failed:
	close(epfd);
udp_ports_init_failed:
	for (int j = 0; j < server_cnt; j++)
		close(server[j].sfd);
	free(server);
direct_out:
	return -1;
}
