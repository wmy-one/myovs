#include <event.h>
#include <pthread.h>

#include "cs_com.h"
#include "bufpool.h"

struct ppstimer {
	struct event evtimer;
	int perbuf_size;
};

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
	struct ppstimer *pt = (struct ppstimer *)arg;

	static long pre_tot_packets = 0;
	static long cur_pps = 0;
	static long cnt_pps = 0;

	/* store 'g_tot_packets' into local variable 'tot_packets', so we
	 * can enure that 'pre_tot_packets' could record the right value.
	 */
	int tot_packets = g_tot_packets;
	cur_pps = (tot_packets - pre_tot_packets);
	pre_tot_packets = tot_packets;

	printf("PPS (%d byte/packet) is %fMpps\n", pt->perbuf_size, cur_pps / 1000000.0);

	// no need lock for this g_avr_pps
	g_avr_pps = g_avr_pps * cnt_pps++ + cur_pps;
	g_avr_pps /= cnt_pps;

	// clock now...
	evtimer_del(&pt->evtimer);
	evtimer_add(&pt->evtimer, &tv);
}

static void udp_cb(const int sock, short int which, void *arg)
{
	struct udp *udp = (struct udp *)arg;
	struct sockaddr_in si_other;
	int slen = sizeof(si_other);

	char *buf = bufpool_get(udp->port);
	if (!buf)
		return;

	if (recvfrom(udp->sfd, buf, udp->buf_size, 0, (struct sockaddr *)&si_other,
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

	bufpool_put(udp->port);

	sec = ntohl(sec);
	usec = ntohl(usec);
	sent_time.tv_sec = sec;
	sent_time.tv_usec = usec;

	recaclu_delay_time(&sent_time, &arrival_time);
}

int main(int argc, char *argv[])
{
	unsigned int i;

	if (argc != 4) {
		ERR("./xxx [Start Port] [Ports Number] [PerBuf Size]");
		return 1;
	}

	unsigned int start_port = atoi(argv[1]);
	unsigned int port_num = atoi(argv[2]);
	unsigned int perbuf_size = atoi(argv[3]);
	assert(start_port + port_num <= 65535);
	assert(perbuf_size >= 12);

	if (bufpool_init(perbuf_size, start_port, port_num) < 0)
		return -1;

	// init server udp ports
	struct udp *server = udp_ports_init((const char *)INADDR_ANY, start_port,
										port_num, perbuf_size);
	assert(server != NULL);

	// create the event base handler and init the event
	base = event_base_new();
	event_init();

	// create the signal event for catching interrupts
	struct event *evsignal[3];
	evsignal[0] = evsignal_new(base, SIGINT, signal_cb, base);
	event_add(evsignal[0], NULL);
	evsignal[1] = evsignal_new(base, SIGHUP, signal_cb, base);
	event_add(evsignal[1], NULL);
	evsignal[2] = evsignal_new(base, SIGTERM, signal_cb, base);
	event_add(evsignal[2], NULL);

	// create the clock timer event for caculate PPS
	struct timeval tv = { .tv_sec = 1, tv.tv_usec = 0};
	struct ppstimer pt = { .perbuf_size = perbuf_size };
	evtimer_set(&pt.evtimer, clock_cb, &pt);
	event_base_set(base, &pt.evtimer);
	evtimer_add(&pt.evtimer, &tv);

	// create the io event for listen mutilate ports
	struct event **evio = (struct event **)malloc(port_num * (sizeof(*evio)));
	for (i = 0; i < port_num; i++) {
		evio[i] = event_new(base, server[i].sfd, EV_READ | EV_PERSIST, udp_cb, &server[i]);
		event_add(evio[i], NULL);
	}

	// loop now...
	event_base_dispatch(base);

	printf("--------------------------------------\n");
	printf("Max Latency     = %.3fms\n", max_latency / 1000.0);
	printf("Min Latency     = %.3fms\n", min_latency / 1000.0);
	printf("Average Latency = %.3fms\n", recaclu_delay_time(NULL, NULL) / 1000.0);
	printf("Average PPS (%d byte/packet) = %fMpps\n", perbuf_size, g_avr_pps / 1000000.0);

	// delete the events rightly
	evtimer_del(&pt.evtimer);
	for (i = 0; i < sizeof(evsignal) / sizeof(evsignal[0]); i++)
		event_free(evsignal[i]);
	for (i = 0; i < port_num; i++)
		event_free(evio[i]);
	event_base_free(base);

	udp_ports_deinit(server, port_num);
	return 0;
}
