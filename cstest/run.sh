#! /bin/bash

ROOT=`dirname "${BASH_SOURCE[0]}"`
SERVER_IP=192.168.2.101
SERVER_PORT=6738
RUN_CLIENT=y
RUN_SERVER=n

PREFIX_CMD=

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
		-s)
			RUN_SERVER=y
			RUN_CLIENT=n
			;;
		-c)
			RUN_CLIENT=y
			RUN_SERVER=n
			;;
		-pcmd)
			if [ -n $2 ]; then
				PREFIX_CMD=$2
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

echo "[DBG]: PREFIX_CMD = ${PREFIX_CMD}"
echo "[DBG]: RUN_SERVER = ${RUN_SERVER}"
echo "[DBG]: RUN_CLIENT = ${RUN_CLIENT}"
echo "[DBG]: SERVER_IP = ${SERVER_IP}"
echo "[DBG]: SERVER_PORT = ${SERVER_PORT}"

if [ $RUN_SERVER = 'y' ]; then
	${PREFIX_CMD} ${ROOT}/out/serverudp $SERVER_PORT
else
	${PREFIX_CMD} ${ROOT}/out/clientudp $SERVER_IP $SERVER_PORT
fi

