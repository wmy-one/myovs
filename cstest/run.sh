#! /bin/bash

ROOT=`dirname "${BASH_SOURCE[0]}"`
SERVER_IP=10.1.1.4  # default match tap1
SERVER_PORT=6738
SERVER_PORT_NUM=100

VERBOSE=n
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
		-spn)
			if [ -n $2 ]; then
				SERVER_PORT_NUM=$2
				shift
			fi
			;;
		-v)
			VERBOSE=y
			;;
		*)
			echo "$0 [-spi <val>] [-sport <val>]"
			exit
			;;
	esac
	shift
done

if [ $VERBOSE = y ]; then
	echo "[DBG]: PREFIX_CMD = ${PREFIX_CMD}"
	echo "[DBG]: RUN_SERVER = ${RUN_SERVER}"
	echo "[DBG]: RUN_CLIENT = ${RUN_CLIENT}"
	echo "[DBG]: SERVER_IP = ${SERVER_IP}"
	echo "[DBG]: SERVER_PORT = ${SERVER_PORT}"
	echo "[DBG]: SERVER_PORT_NUM = ${SERVER_PORT_NUM}"
fi

if [ $RUN_SERVER = 'y' ]; then
	${PREFIX_CMD} ${ROOT}/out/serverudp $SERVER_PORT $SERVER_PORT_NUM
else
	${PREFIX_CMD} ${ROOT}/out/clientudp $SERVER_IP $SERVER_PORT $SERVER_PORT_NUM
fi

