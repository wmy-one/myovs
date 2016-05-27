#ifndef __UDP_H__
#define __UDP_H__

#include <arpa/inet.h>

struct udp {
	struct sockaddr_in si;
	int sfd;
	int port;
};

struct udp *udp_ports_init(const char *server_ip, int start_port, int port_num);
int udp_epoll_init(struct udp *udp, int len, int event);
void udp_ports_deinit(struct udp *udp, int len);

#endif  /* __UDP_H__ */
