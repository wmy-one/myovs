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

trap 'exit 1' SIGINT
trap 'kill $(jobs -p) &> /dev/null' EXIT

# forcely clean up network environment
bash ovs_destroy.sh

# add the namespaces
ip netns add ns1
ip netns add ns2

# create the switch
ovs-vsctl add-br $BRIDGE

###### PORT 1
# create a port pair
ip link add tap1 type veth peer name ovs-tap1
# attach one side to ovs
ovs-vsctl add-port $BRIDGE ovs-tap1
# attach the other side to namespace
ip link set tap1 netns ns1
# set the ports to up
ip netns exec ns1 ip link set dev tap1 up
ip link set dev ovs-tap1 up


###### PORT 2
# create a port pair
ip link add tap2 type veth peer name ovs-tap2
# attach one side to ovs
ovs-vsctl add-port $BRIDGE ovs-tap2
# attach the other side to namespace
ip link set tap2 netns ns2
# set the ports to up
ip netns exec ns2 ip link set dev tap2 up
ip link set dev ovs-tap2 up


########### Assgin IP address to "tap1" and "tap2"
ip netns exec ns1 ip addr add 10.1.1.4/24 dev tap1
ip netns exec ns2 ip addr add 10.1.1.5/24 dev tap2

########### Add the flow tables
function add_port_to_flow {
	PORT=$1
	sudo ovs-ofctl add-flow $BRIDGE in_port=2,idle_timeout=180,priority=33001,udp,tp_dst=$PORT,actions=output:1
}
for i in {6738..6838}; do
	add_port_to_flow $i
done

echo -e "\033[32m------------------                             ------------------	\033[0m"
echo -e "\033[32m|                |                             |                |	\033[0m"
echo -e "\033[32m|         -------|Veth Pair           Veth Pair|-------         |	\033[0m"
echo -e "\033[32m|         | tap1 |------                -------| tap2 |         |	\033[0m"
echo -e "\033[32m|         -------|     |                |      |-------         |	\033[0m"
echo -e "\033[32m|                |     |                |      |                |	\033[0m"
echo -e "\033[32m|Namespace 'ns1' |     |                |      | Namespace 'ns2'|	\033[0m"
echo -e "\033[32m------------------     |                |      ------------------	\033[0m"
echo -e "\033[32m                       v                v							\033[0m"
echo -e "\033[32m                   ---------------------------						\033[0m"
echo -e "\033[32m                   |ovs-tap1         ovs-tap2|						\033[0m"
echo -e "\033[32m                   |                         |						\033[0m"
echo -e "\033[32m                   |       Openvswith        |						\033[0m"
echo -e "\033[32m                   ---------------------------						\033[0m"
