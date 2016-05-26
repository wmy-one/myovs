#! /bin/bash
# Author: yakir.yang@qq.com

touch /etc &> /dev/null
if [ $? -eq 1 ]; then
	echo -e "\033[31m$0: Premission denied\033[0m"
	exit
fi

trap 'exit' SIGINT
trap 'kill $(jobs -p) &> /dev/null' EXIT

BRIDGE=ykk-ovs-test

# forcely clean up network environment
ovs-vsctl del-br $BRIDGE &> /dev/null
ip link delete ovs-tap1  &> /dev/null
ip link delete ovs-tap2  &> /dev/null
ip netns delete ns1      &> /dev/null
ip netns delete ns2      &> /dev/null

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
	sudo ovs-ofctl add-flow $BRIDGE idle_timeout=180,priority=33001,udp,tp_dst=$PORT,actions=output:1
}
for i in {6738..6838}; do
	add_port_to_flow $i
done


########### Print some debug message
echo -e "\033[32m================> Check the network status in \"ns1\" \033[0m"
ip netns exec ns1 ifconfig -a tap1
ip netns exec ns1 route -n
echo -e "\033[32m================> Check the network status in \"ns2\" \033[0m"
ip netns exec ns2 ifconfig -a tap2
ip netns exec ns2 route -n


########### Run ping test between tap1 and tap2
echo -e "\033[32m================> Start ping test between \"tap1\" and \"tap2\" \033[0m"
ip netns exec ns1 ping -c 2 10.1.1.5
if [ $? -eq 1 ]; then
	echo -e "\033[31mPING TEST FAILED\033[0m"
	exit
fi

echo -e "\033[32m================> Here are the network topology of this script.	\033[0m"
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


################### Run UDP client/server test program
ip netns exec ns1 ./cstest/run.sh -s &> /dev/null &
ip netns exec ns2 ./cstest/run.sh -c &> /dev/null &


################### Print the target "pps"
current_packets=(0)
previous_packets=(0)
ports_pps=(0)
total_pps=0

while true; do
	sudo ovs-ofctl dump-flows $BRIDGE > .result
	sed -n '/tp_dst/p' .result | awk '{print $4}' | sed 's/[^0-9]//g' > .result.ports

	port=0
	total_pps=0

	while read line; do
		current_packets[$port]=$line
		ports_pps[$port]=$((${current_packets[$port]} - ${previous_packets[$port]-0}));

		total_pps=$((${total_pps} + ${ports_pps[$port]}));
		previous_packets[$port]=${current_packets[$port]};

		let 'port += 1'
	done < .result.ports

	date
	total_pps=`echo "scale=5; $total_pps/1024/1024" | bc`
	echo -e "\033[32mPorts's pps (1024 byte/packet) = ${total_pps}Mpps   \033[0m"

	sleep 1;
done;
