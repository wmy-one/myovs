#ifndef __UDP_H__
#define __UDP_H__

#include <arpa/inet.h>

struct udp {
	struct sockaddr_in si;
	int sfd;
	int port;
};

struct udp *udp_ports_init(const char *server_ip, unsigned int start_port,
						   unsigned int port_num);
int udp_epoll_init(struct udp *udp, unsigned int len, int event);
void udp_ports_deinit(struct udp *udp, unsigned int len);

#endif  /* __UDP_H__ */
