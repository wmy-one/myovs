#! /bin/bash
# Author: yakir.yang@qq.com

touch /etc &> /dev/null
if [ $? -eq 1 ]; then
	echo -e "\033[31m$0: Premission denied\033[0m"
	exit
fi

if [ -z $BRIDGE ]; then
	BRIDGE=ykk-ovs-test
fi

# forcely clean up network environment
ovs-vsctl del-br $BRIDGE &> /dev/null
ip link delete ovs-tap1  &> /dev/null
ip link delete ovs-tap2  &> /dev/null
ip netns delete ns1      &> /dev/null
ip netns delete ns2      &> /dev/null
