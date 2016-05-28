#include <assert.h>
#include <event.h>
#include <pthread.h>

#include "cs_com.h"

// Globol libevent base handler
static struct event_base *base;

// used for cacluating Packet Per Second
static unsigned long long g_tot_packets = 0;
static unsigned long g_avr_pps = 0;

static unsigned long min_latency = 9999;
static unsigned long max_latency = 0;
pthread_mutex_t mutex_latency = PTHREAD_MUTEX_INITIALIZER;

static unsigned long long recaclu_delay_time(struct timeval *st,
								 struct timeval *et)
{
	static unsigned long long delay_times = 0;
	static unsigned long transit_cnt = 0;
	unsigned long time1, time2, timediff;

	if ((!st || !et) && transit_cnt)
		return delay_times / transit_cnt;

	time1 = st->tv_sec * 1000000UL + st->tv_usec;
	time2 = et->tv_sec * 1000000UL + et->tv_usec;
	timediff = (time1 > time2) ? (time1 - time2) : (time2 - time1);

	pthread_mutex_lock(&mutex_latency);
	if (timediff < min_latency)
		min_latency = timediff;
	else if (timediff > max_latency)
		max_latency = timediff;
	pthread_mutex_unlock(&mutex_latency);

	__sync_fetch_and_add(&delay_times, timediff);
	__sync_fetch_and_add(&transit_cnt, 1);
	return 0;
}

// terminate the event_base_dispatch() loop in main()
static void signal_cb(evutil_socket_t fd, short event, void *arg)
{
	struct event_base *base = arg;
	event_base_loopbreak(base);
}

// handler with PPS things
void clock_cb(int fd, short event, void *arg)
{
	struct timeval tv = { .tv_sec = 1, tv.tv_usec = 0};
	struct event *evtimer = (struct event *)arg;

	static long pre_tot_packets = 0;
	static long cur_pps = 0;
	static long cnt_pps = 0;

	/* store 'g_tot_packets' into local variable 'tot_packets', so we
	 * can enure that 'pre_tot_packets' could record the right value.
	 */
	int tot_packets = g_tot_packets;
	cur_pps = (tot_packets - pre_tot_packets);
	pre_tot_packets = tot_packets;

	printf("PPS (1024 byte/packet) is %fMpps\n", cur_pps / 1000000.0);

	// no need lock for this g_avr_pps
	g_avr_pps = g_avr_pps * cnt_pps++ + cur_pps;
	g_avr_pps /= cnt_pps;

	// clock now...
	evtimer_del(evtimer);
	evtimer_add(evtimer, &tv);
}

static void udp_cb(const int sock, short int which, void *arg)
{
	struct udp *udp = (struct udp *)arg;
	struct sockaddr_in si_other;
	int slen = sizeof(si_other);
	char buf[BUFLEN];

	if (recvfrom(udp->sfd, buf, BUFLEN, 0, (struct sockaddr *)&si_other,
				(socklen_t *)&slen) == -1) {
		ERR("udp[%d] port[%d] receive failed\n", udp->sfd, udp->port);
		return;
	}

	if (check_recv_buf_head(buf) < 0) {
		ERR("udp[%d] port[%d] data check failed\n", udp->sfd, udp->port);
		return;
	}

	__sync_fetch_and_add(&g_tot_packets, 1);

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

int main(int argc, char *argv[])
{
	if (argc != 3) {
		ERR("./xxx [Start Port] [Ports Number]");
		return 1;
	}

	int start_port = atoi(argv[1]);
	int port_num = atoi(argv[2]);
	assert(start_port + port_num <= 65535);

	// init server udp ports
	struct udp *server = udp_ports_init((const char *)INADDR_ANY, start_port, port_num);
	assert(server != NULL);

	// create the event base handler and init the event
	base = event_base_new();
	event_init();

	// create the signal event for catching interrupts
	struct event *evsignal;
	evsignal = evsignal_new(base, SIGINT, signal_cb, base);
	evsignal = evsignal_new(base, SIGHUP, signal_cb, base);
	evsignal = evsignal_new(base, SIGTERM, signal_cb, base);
	event_add(evsignal, NULL);

	// create the clock timer event for caculate PPS
	struct timeval tv = { .tv_sec = 1, tv.tv_usec = 0};
	struct event evtimer;
	evtimer_set(&evtimer, clock_cb, &evtimer);
	event_base_set(base, &evtimer);
	evtimer_add(&evtimer, &tv);

	// create the io event for listen mutilate ports
	struct event **evio = (struct event **)malloc(port_num * (sizeof(*evio)));
	for (int i = 0; i < port_num; i++) {
		evio[i] = event_new(base, server[i].sfd, EV_READ | EV_PERSIST, udp_cb, &server[i]);
		event_add(evio[i], NULL);
	}

	// loop now...
	event_base_dispatch(base);

	printf("Average pps (1024 byte/packet) = %fMpps\n", g_avr_pps / 1000000.0);
	printf("Average Latency = %.3fms\n", recaclu_delay_time(NULL, NULL) / 1000.0);
	printf("Min Latency     = %.3fms\n", min_latency / 1000.0);
	printf("Max Latency     = %.3fms\n", max_latency / 1000.0);

	udp_ports_deinit(server, port_num);
	return 0;
}
