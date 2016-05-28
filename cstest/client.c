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

int main(int argc, char *argv[])
{
	struct udp *client;
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

	int epfd = udp_epoll_init(client, port_num, EPOLLOUT);
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
				struct udp *udp = (struct udp *)events[i].data.ptr;

				init_send_buffer(buf);

				if (sendto(udp->sfd, buf, BUFLEN, 0, (struct sockaddr *)&udp->si,
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

epoll_add_failed:
	close(epfd);
udp_ports_init_failed:
	udp_ports_deinit(client, port_num);
direct_out:
	return -1;
}
