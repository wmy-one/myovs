OUT=out

all: clientudp serverudp

clientudp: client.c
	g++ client.c -o ${OUT}/clientudp

serverudp: server.c
	g++ server.c -o ${OUT}/serverudp

clean:
	rm -rf out/*
