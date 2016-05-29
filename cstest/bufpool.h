#ifndef __BUF_POLL_H__
#define __BUF_POLL_H__

int bufpool_init(unsigned int perbuf_size, unsigned int start, unsigned int ports);
void bufpool_deinit(void);
char *bufpool_get(unsigned int port);
void bufpool_put(unsigned int port);

#endif
