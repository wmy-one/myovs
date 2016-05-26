#include "cs_com.h"

static int stop = 0;

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
		ERR("failed at socket()\n");
		return sfd;
	}

	int opt = SO_REUSEADDR;
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	bzero(&si_me, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);

	si_me.sin_port = htons(port);
	if (bind(sfd, (const struct sockaddr *)&si_me, sizeof(si_me)) == -1) {
		ERR("failed at bind()\n");
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
		ERR("failed at server malloc()\n");
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

static int udpin_epoll_init(struct udpserver *server, int len)
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
		ev.data.ptr = &server[i];
		ev.events = EPOLLIN | EPOLLET;
		ret = epoll_ctl(epfd, EPOLL_CTL_ADD, server[i].sfd, &ev);
		if (ret < 0) {
			ERR("failed at EPOLL_CTL_ADD\n");
			close(epfd);
			return -1;
		}
		setnonblocking(server[i].sfd);
	}

	return epfd;
}

static double timeval_diff(struct timeval * tv0, struct timeval * tv1)
{
    double time1, time2;

    time1 = tv0->tv_sec + (tv0->tv_usec / 1000000.0);
    time2 = tv1->tv_sec + (tv1->tv_usec / 1000000.0);

    time1 = time1 - time2;
    if (time1 < 0)
        time1 = -time1;
    return time1;
}

// ATTENTION: this is not thread safe here
static double recaclu_delay_time(struct timeval *sent_time,
								 struct timeval *arrival_time)
{
	static double delay_times = 0;
	static int transit_cnt = 0;

	if (!sent_time || !arrival_time)
		return delay_times / transit_cnt;

	delay_times += timeval_diff(sent_time, arrival_time);
	return delay_times / ++transit_cnt;
}

static inline void udp_ports_deinit(struct udpserver *server, int len)
{
	for (int j = 0; j < len; j++)
		close(server[j].sfd);

	free(server);
}

static void loop_over(int sig)
{
	stop = 1;
}

int main(int argc, char *argv[])
{
	char buf[BUFLEN];
	int i;

	if (argc != 3) {
		ERR("./xxx [Start Port] [Ports Number]");
		return 1;
	}

	int start_port = atoi(argv[1]);
	int port_num = atoi(argv[2]);
	ERROR_CHECK(start_port + port_num <= 65535, direct_out);

	struct udpserver *server = udp_ports_init(start_port, port_num);
	ERROR_CHECK(server != NULL, direct_out);

	int epfd = udpin_epoll_init(server, port_num);
	ERROR_CHECK(epfd > 0, udp_ports_init_failed);

	struct epoll_event *events;
	events = (struct epoll_event *)malloc(sizeof(*events) * port_num);
	ERROR_CHECK(events != NULL, epoll_add_failed);

	addsingal(SIGHUP, loop_over);
	addsingal(SIGINT, loop_over);
	addsingal(SIGTERM, loop_over);

	struct sockaddr_in si_other;
	int slen = sizeof(si_other);
	while (!stop) {
		int nfds = epoll_wait(epfd, events, port_num, -1);
		for (i = 0; i < nfds; i++) {
			if (events[i].events & EPOLLIN) {
				struct udpserver *svr = (struct udpserver *)events[i].data.ptr;

				if (recvfrom(svr->sfd, buf, BUFLEN, 0, (struct sockaddr *)&si_other,
							 (socklen_t *)&slen) == -1) {
					ERR("udp[%d] port[%d] receive failed\n", svr->sfd, svr->port);
				} else {
					if (check_recv_buf_head(buf) < 0) {
						ERR("udp[%d] port[%d] data check failed\n", svr->sfd, svr->port);
						continue;
					}

					struct timeval sent_time, arrival_time;
					uint32_t sec, usec;
					gettimeofday(&arrival_time, 0);
					memcpy(&sec, buf+4, sizeof(sec));
					memcpy(&usec, buf+8, sizeof(usec));
					sec = ntohl(sec);
					usec = ntohl(usec);
					sent_time.tv_sec = sec;
					sent_time.tv_usec = usec;

					recaclu_delay_time(&sent_time, &arrival_time);
				}
			}
		}
	}

	printf("Average delay time = %fs\n", recaclu_delay_time(NULL, NULL));

	close(epfd);
	udp_ports_deinit(server, port_num);

	return 0;

epoll_add_failed:
	close(epfd);
udp_ports_init_failed:
	udp_ports_deinit(server, port_num);
direct_out:
	return -1;
}
