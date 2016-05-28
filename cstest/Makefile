OUT = out
CFLAGS = -Wall -D_POSIX_C_SOURCE=200112L -D_BSD_SOURCE -std=c99
LIBS = -levent

all: clientudp serverudp

serverudp: server.c udp.c bufpool.c
	$(CC) $(CFLAGS) -o ${OUT}/$@ $^ $(LIBS)

clientudp: client.c udp.c
	$(CC) $(CFLAGS) -o ${OUT}/$@ $^ $(LIBS)

clean:
	rm -rf out/*
