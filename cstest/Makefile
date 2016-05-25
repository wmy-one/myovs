OUT=out

all: clientudp serverudp

clientudp: client.c
	gcc -D_BSD_SOURCE -std=c99 client.c -o ${OUT}/clientudp

serverudp: server.c
	gcc -D_BSD_SOURCE -std=c99 server.c -o ${OUT}/serverudp

clean:
	rm -rf out/*
