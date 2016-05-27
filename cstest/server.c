#include "cs_com.h"

static int stop = 0;

static void loop_over(int sig)
{
	sig = sig;
	stop = 1;
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

	transit_cnt++;
	delay_times += timeval_diff(sent_time, arrival_time);
	return 0;
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

	struct udp *server;
	server = udp_ports_init((const char *)INADDR_ANY, start_port, port_num);
	ERROR_CHECK(server != NULL, direct_out);

	int epfd = udp_epoll_init(server, port_num, EPOLLIN);
	ERROR_CHECK(epfd > 0, udp_ports_init_failed);

	struct epoll_event *events;
	events = (struct epoll_event *)malloc(sizeof(*events) * port_num);
	ERROR_CHECK(events != NULL, epoll_add_failed);

	addsingal(SIGHUP, loop_over);
	addsingal(SIGINT, loop_over);
	addsingal(SIGTERM, loop_over);

	static int recv_cnt = 0;

	struct sockaddr_in si_other;
	int slen = sizeof(si_other);
	while (!stop) {
		int nfds = epoll_wait(epfd, events, port_num, -1);
		for (i = 0; i < nfds; i++) {
			if (events[i].events & EPOLLIN) {
				struct udp *udp = (struct udp *)events[i].data.ptr;

				if (recvfrom(udp->sfd, buf, BUFLEN, 0, (struct sockaddr *)&si_other,
							 (socklen_t *)&slen) == -1) {
					ERR("udp[%d] port[%d] receive failed\n", udp->sfd, udp->port);
				} else {
					if (check_recv_buf_head(buf) < 0) {
						ERR("udp[%d] port[%d] data check failed\n", udp->sfd, udp->port);
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
					recv_cnt++;
				}
			}
		}
	}

	printf("---------SERVER----------- recv_cnt = %d\n", recv_cnt);
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
