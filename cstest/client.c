#include "cs_com.h"

static int stop = 0;

static void loop_over(int sig)
{
	sig = sig;
	stop = 1;
}

static int addsingal(int sig, void (*handler)(int))
{
	struct sigaction sa;
	bzero(&sa, sizeof(sa));
	sa.sa_handler = handler;
	sa.sa_flags |= SA_RESTART;
	sigfillset(&sa.sa_mask);
	return sigaction(sig, &sa, NULL);
}

void init_send_buffer(char *buf, unsigned int size)
{
	struct timeval before;
	uint32_t  sec, usec;

	if (!buf || size < 8)
		return;

	gettimeofday(&before, 0);
	sec = htonl(before.tv_sec);
	usec = htonl(before.tv_usec);

	bzero(buf, size);
	set_send_buf_head(buf);
	memcpy(buf+4, &sec, sizeof(sec));
	memcpy(buf+8, &usec, sizeof(usec));
}

int main(int argc, char *argv[])
{
	struct udp *client;
	int i;

	if (argc != 5) {
		ERR("./xxx [Host IP] [Start port] [ports Number] [PerBuf Size]");
		return 1;
	}

	const char *server_ip = argv[1];
	unsigned int server_port = atoi(argv[2]);
	unsigned int port_num = atoi(argv[3]);
	unsigned int perbuf_size = atoi(argv[4]);
	assert(server_port + port_num <= 65535);
	assert(perbuf_size >= 8);

	client = udp_ports_init(server_ip, server_port, port_num, perbuf_size);
	assert(client != NULL);

	int epfd = udp_epoll_init(client, port_num, EPOLLOUT);
	if (epfd <= 0) {
		udp_ports_deinit(client, port_num);
		return -1;
	}

	struct epoll_event *events;
	events = (struct epoll_event *)malloc(sizeof(*events) * port_num);
	if (events == NULL) {
		close(epfd);
		udp_ports_deinit(client, port_num);
		return -1;
	}

	addsingal(SIGHUP, loop_over);
	addsingal(SIGINT, loop_over);
	addsingal(SIGTERM, loop_over);

	char *buf = (char *)malloc(perbuf_size);
	while (!stop) {
		int nfds = epoll_wait(epfd, events, port_num, -1);
		for (i = 0; i < nfds; i++) {
			if (events[i].events & EPOLLOUT) {
				struct udp *udp = (struct udp *)events[i].data.ptr;

				init_send_buffer(buf, udp->buf_size);

				if (sendto(udp->sfd, buf, udp->buf_size, 0, (struct sockaddr *)&udp->si,
						   sizeof(struct sockaddr_in)) == -1) {
					ERR("udp[%d] port[%d] send failed\n", udp->sfd, udp->port);
					break;
				}
			}
		}
	}

	udp_ports_deinit(client, port_num);
	close(epfd);

	return 0;
}
