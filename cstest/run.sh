#! /bin/bash

SERVER_IP=192.168.2.101
SERVER_PORT=6738

while [ -n "$1" ]
do
	case "$1" in
		-sip)
			if [ -n $2 ]; then
				SERVER_IP=$2
				shift
			fi
			;;
		-sport)
			if [ -n $2 ]; then
				SERVER_PORT=$2
				shift
			fi
			;;
		*)
			echo "$0 [-spi <val>] [-sport <val>]"
			exit
			;;
	esac
	shift
done

echo "[DBG]: Server IP is ${SERVER_IP}"
echo "[DBG]: Server Port is ${SERVER_PORT}"

./out/serverudp $SERVER_PORT &
./out/clientudp $SERVER_IP $SERVER_PORT

