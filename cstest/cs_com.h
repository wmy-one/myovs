#ifndef __CS_COMMON_HEAD__
#define __CS_COMMON_HEAD__

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFLEN		1024
#define TEST_DATA	'a'

#define ERROR_CHECK(con, destroy)										\
		if (!(con)) {													\
			printf("[ERROR]: Failed at %s:%d\n", __func__, __LINE__);	\
			goto destroy;												\
		}

static inline int setnonblocking(int fd)
{
	int old_opt = fcntl(fd, F_GETFL);
	int new_opt = old_opt | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_opt);
	return old_opt;
}

#endif /* __CS_COMMON_HEAD__ */
