#include "cs_com.h"

struct udpclient {
	struct sockaddr_in si_other;
	int port;
	int sfd;
};

static int udp_port_init(struct udpclient *client, const char *server_ip,
						 int port)
{
	if (client == NULL)
		return -1;

	int sfd;

	sfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sfd == -1) {
		printf("[ERROR]: failed at socket()\n");
		return sfd;
	}

	bzero(&client->si_other, sizeof(client->si_other));
	client->si_other.sin_family = AF_INET;
	client->si_other.sin_port = htons(port);
	if (inet_aton(server_ip, &client->si_other.sin_addr) == 0) {
		printf("[ERROR]: failed at inet_aton()\n");
		close(sfd);
		return -1;
	}

	client->sfd = sfd;

	return 0;
}

static struct udpclient *udp_ports_init(const char *server_ip, int server_port,
										int port_num)
{
	struct udpclient *client;
	int ret;
	int i;

	client = (struct udpclient *)malloc(sizeof(*client) * port_num);
	if (client == NULL) {
		printf("[ERROR]: failed at client malloc()\n");
		return NULL;
	}

	for (i = 0; i < port_num; i++) {
		ret = udp_port_init(&client[i], server_ip, server_port + i);
		client[i].port = server_port + i;
		if (ret < 0)
			goto udp_port_init_failed;
	}

	return client;

udp_port_init_failed:
	while (i >= 0)
		close(client[--i].sfd);

	return NULL;
}

static void udp_ports_deinit(struct udpclient *client, int len)
{
	for (int j = 0; j < len; j++)
		close(client[j].sfd);

	free(client);
}

static int udpout_epoll_init(struct udpclient *client, int len)
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
		ev.data.ptr = &client[i];
		ev.events = EPOLLOUT | EPOLLET;
		ret = epoll_ctl(epfd, EPOLL_CTL_ADD, client[i].sfd, &ev);
		if (ret < 0) {
			printf("[ERROR]: failed at EPOLL_CTL_ADD\n");
			close(epfd);
			return -1;
		}
		setnonblocking(client[i].sfd);
	}

	return epfd;
}

static inline void udpout_epoll_deinit(int epfd)
{
	close(epfd);
}

int main(int argc, char *argv[])
{
	struct udpclient *client;
	int client_cnt = 0;
	char buf[BUFLEN];
	int i;

	if (argc != 4) {
		perror("ERROR: ./xxx [Host IP] [Start Port] [Ports Number]");
		return 1;
	}

	memset(buf, TEST_DATA, sizeof(buf));

	const char *server_ip = argv[1];
	int server_port = atoi(argv[2]);
	int port_num = atoi(argv[3]);
	ERROR_CHECK(server_port + port_num <= 65535, direct_out);

	client = udp_ports_init(server_ip, server_port, port_num);
	ERROR_CHECK(client != NULL, direct_out);

	int epfd = udpout_epoll_init(client, port_num);
	ERROR_CHECK(epfd > 0, direct_out);

	struct epoll_event *events;
	events = (struct epoll_event *)malloc(sizeof(*events) * port_num);
	ERROR_CHECK(events != NULL, epoll_add_failed);

	while (1) {
		int nfds = epoll_wait(epfd, events, port_num, -1);
		for (i = 0; i < nfds; i++) {
			if (events[i].events & EPOLLOUT) {
				struct udpclient *clt = (struct udpclient *)events[i].data.ptr;
				if (sendto(clt->sfd, buf, BUFLEN, 0, (struct sockaddr *)&clt->si_other,
						   sizeof(struct sockaddr_in)) == -1) {
					printf("[ERROR]: udp[%d] port[%d] send failed\n", clt->sfd, clt->port);
				} else {
					// TODO: need fix the losing packet problem
					printf("[DBG]: udp[%d] port[%d] send success\n", clt->sfd, clt->port);
				}
			}
		}
	}

	udp_ports_deinit(client, port_num);
	udpout_epoll_deinit(epfd);

	return 0;

epoll_add_failed:
	udpout_epoll_deinit(epfd);
udp_ports_init_failed:
	udp_ports_deinit(client, port_num);
direct_out:
	return -1;
}
