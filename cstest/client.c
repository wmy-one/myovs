#include "cs_com.h"

static int stop = 0;

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
		ERR("failed at socket()\n");
		return sfd;
	}

	bzero(&client->si_other, sizeof(client->si_other));
	client->si_other.sin_family = AF_INET;
	client->si_other.sin_port = htons(port);
	if (inet_aton(server_ip, &client->si_other.sin_addr) == 0) {
		ERR("failed at inet_aton()\n");
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
		ERR("failed at client malloc()\n");
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

static inline void udp_ports_deinit(struct udpclient *client, int len)
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
		ERR("failed at epoll_create()\n");
		return -1;
	}

	for (int i = 0; i < len; i++) {
		ev.data.ptr = &client[i];
		ev.events = EPOLLOUT | EPOLLET;
		ret = epoll_ctl(epfd, EPOLL_CTL_ADD, client[i].sfd, &ev);
		if (ret < 0) {
			ERR("failed at EPOLL_CTL_ADD\n");
			close(epfd);
			return -1;
		}
		setnonblocking(client[i].sfd);
	}

	return epfd;
}

void init_send_buffer(char *buf)
{
	struct timeval before;
	uint32_t  sec, usec;

	gettimeofday(&before, 0);
	sec = htonl(before.tv_sec);
	usec = htonl(before.tv_usec);

	bzero(buf, BUFLEN);
	set_send_buf_head(buf);
	memcpy(buf+4, &sec, sizeof(sec));
	memcpy(buf+8, &usec, sizeof(usec));
}

static void loop_over(int sig)
{
	stop = 1;
}

int main(int argc, char *argv[])
{
	struct udpclient *client;
	char buf[BUFLEN];
	int i;

	if (argc != 4) {
		ERR("./xxx [Host IP] [Start port] [ports Number]");
		return 1;
	}

	const char *server_ip = argv[1];
	int server_port = atoi(argv[2]);
	int port_num = atoi(argv[3]);
	ERROR_CHECK(server_port + port_num <= 65535, direct_out);

	client = udp_ports_init(server_ip, server_port, port_num);
	ERROR_CHECK(client != NULL, direct_out);

	int epfd = udpout_epoll_init(client, port_num);
	ERROR_CHECK(epfd > 0, udp_ports_init_failed);

	struct epoll_event *events;
	events = (struct epoll_event *)malloc(sizeof(*events) * port_num);
	ERROR_CHECK(events != NULL, epoll_add_failed);

	addsingal(SIGHUP, loop_over);
	addsingal(SIGINT, loop_over);
	addsingal(SIGTERM, loop_over);

	while (!stop) {
		int nfds = epoll_wait(epfd, events, port_num, -1);
		for (i = 0; i < nfds; i++) {
			if (events[i].events & EPOLLOUT) {
				struct udpclient *clt = (struct udpclient *)events[i].data.ptr;

				init_send_buffer(buf);

				if (sendto(clt->sfd, buf, BUFLEN, 0, (struct sockaddr *)&clt->si_other,
						   sizeof(struct sockaddr_in)) == -1)
					ERR("udp[%d] port[%d] send failed\n", clt->sfd, clt->port);
			}
		}
	}

	udp_ports_deinit(client, port_num);
	close(epfd);

	return 0;

epoll_add_failed:
	close(epfd);
udp_ports_init_failed:
	udp_ports_deinit(client, port_num);
direct_out:
	return -1;
}
