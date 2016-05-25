#include "cs_com.h"

int main(int argc, char *argv[])
{
	struct sockaddr_in si_other;
	int slen=sizeof(si_other);
	char buf[BUFLEN];
	int sfd;

	if (argc != 3)
		diep("ERROR: ./xxx HOST_IP HOST_PORT");

	const char *server_ip = argv[1];
	int server_port = atoi(argv[2]);

	sfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sfd == -1)
		diep("socket");

	memset((char *)&si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(server_port);
	if (inet_aton(server_ip, &si_other.sin_addr) == 0)
		diep("inet_aton");

	for (int i = 0; i < BUFLEN; i++)
		buf[i] = TEST_DATA;

	if (sendto(sfd, buf, BUFLEN, 0, (struct sockaddr *)&si_other, slen) == -1)
		diep("sendto");

	close(sfd);

	return 0;
}
