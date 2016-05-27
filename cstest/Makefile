OUT=out

all: clientudp serverudp

clientudp: udp.c client.c
	gcc -Wall -Wextra -D_POSIX_C_SOURCE=200112L -D_BSD_SOURCE -std=c99 udp.c client.c -o ${OUT}/clientudp

serverudp: udp.c server.c
	gcc -Wall -Wextra -D_POSIX_C_SOURCE=200112L -D_BSD_SOURCE -std=c99 udp.c server.c -o ${OUT}/serverudp

clean:
	rm -rf out/*
