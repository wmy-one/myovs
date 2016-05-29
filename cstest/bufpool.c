#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "bufpool.h"

struct bufpool {
	char *buf;
	int ref_cnt;
	pthread_mutex_t mutex;
};

// Globol buffer poll for sockets
static struct bufpool *udpbp;
static unsigned int udpbp_cnt;
static unsigned int start;

int bufpool_init(unsigned int perbuf_size, unsigned int startp, unsigned int ports)
{
	char *buffers;
	if (udpbp)
		return -1;

	buffers = (char *)malloc(perbuf_size * ports);
	if (!buffers)
		return -1;

	udpbp = (struct bufpool *)malloc(sizeof(*udpbp) * ports);
	if (!udpbp)
		return -1;

	udpbp_cnt = ports;
	start = startp;
	memset(udpbp, 0, sizeof(*udpbp) * ports);

	for (unsigned int i = 0; i < ports; i++) {
		udpbp[i].buf = buffers + perbuf_size * i;
		pthread_mutex_init(&udpbp[i].mutex, NULL);
	}

	return 0;
}

char *bufpool_get(unsigned int port)
{
	if (!udpbp || port < start || port > (start + udpbp_cnt))
		return NULL;

	int i = port - start;
	if (__sync_fetch_and_add(&udpbp[i].ref_cnt, 1))
		pthread_mutex_lock(&udpbp[i].mutex);

	return udpbp[i].buf;
}

void bufpool_put(unsigned int port)
{
	if (!udpbp || port < start || port > (start + udpbp_cnt))
		return;

	int i = port - start;
	if (udpbp[i].ref_cnt && __sync_sub_and_fetch(&udpbp[i].ref_cnt, 1))
		pthread_mutex_unlock(&udpbp[i].mutex);
}

void bufpool_deinit(void)
{
	if (!udpbp)
		return;

	free(udpbp[0].buf);
	free(udpbp);
}
