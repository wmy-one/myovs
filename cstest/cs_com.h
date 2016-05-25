#ifndef __CS_COMMON_HEAD__
#define __CS_COMMON_HEAD__

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFLEN		1024
#define TEST_DATA	'a'

void diep(const char *s)
{
	perror(s);
	exit(1);
}

#endif /* __CS_COMMON_HEAD__ */
