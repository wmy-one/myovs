#ifndef __CS_COMMON_HEAD__
#define __CS_COMMON_HEAD__

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/time.h>

#include "udp.h"

#define ERR(format, ...) fprintf(stderr, "\e[1;31m[%s:%d]\e[0m" format "\n", __FILE__, __LINE__,  ##__VA_ARGS__)

#ifdef DEBUG
#define DBG(format, ...) fprintf(stderr, "\e[1;32m[%s:%d]\e[0m" format "\n", __FILE__, __LINE__,  ##__VA_ARGS__)
#else
#define DBG(format, ...)
#endif

#define ERROR_CHECK(con, destroy)										\
		if (!(con)) {													\
			printf("[ERROR]: Failed at %s:%d\n", __func__, __LINE__);	\
			goto destroy;												\
		}

#define BUFLEN			1024
#define TEST_DATA		((char)0xAA)

static inline int check_recv_buf_head(const char *buf)
{
	if (buf[0] != TEST_DATA || buf[1] != ~TEST_DATA ||
		buf[2] != TEST_DATA || buf[3] != ~TEST_DATA)
		return -1;
	return 0;
}

static inline void set_send_buf_head(char *buf)
{
	buf[0] = TEST_DATA; buf[1] = ~TEST_DATA;
	buf[2] = TEST_DATA; buf[3] = ~TEST_DATA;
}

#endif /* __CS_COMMON_HEAD__ */
