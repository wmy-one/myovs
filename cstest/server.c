#include "cs_com.h"

#define PRINT_BUF(buf, len)					\
	for (int i = 0; i < (len); i++)			\
		printf("%c", buf[i]);				\

int main(int argc, char *argv[])
{
	struct sockaddr_in si_me, si_other;
	int slen = sizeof(si_other);
	char buf[BUFLEN];
	int sfd;
	int i;

	if (argc != 2)
		diep("ERROR: ./xxx LISTEN_PORT");

	int listen_port = atoi(argv[1]);

	sfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sfd == -1)
		diep("socket");

	memset((char *)&si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(listen_port);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sfd, (const sockaddr *)&si_me, sizeof(si_me)) == -1)
		diep("bind");

	if (recvfrom(sfd, buf, BUFLEN, 0, (struct sockaddr *)&si_other,
				 (socklen_t *)&slen) == -1)
		diep("recvfrom");

	for (i = 0; i < BUFLEN; i++)
		if (buf[i] != TEST_DATA)
			break;

	if (i == BUFLEN) {
		printf("Test passed, data match\n");
	} else {
		printf("Test failed, data mismatch\n");
		PRINT_BUF(buf, BUFLEN);
	}

	close(sfd);

	return 0;
}
