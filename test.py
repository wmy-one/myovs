#! /usr/bin/python
# Author: yakir.yang@qq.com

import argparse
import commands
import os
import re
import signal
import sys
import time

if os.geteuid() != 0:
    print '\033[1;31;40mThis program must be run as root. Aborting.\033[0m'
    sys.exit(1)

parser = argparse.ArgumentParser(prog='PROG')
parser = argparse.ArgumentParser(description="An experiment about openvswitch.")
parser.add_argument("-v", help="enable print some debug messages", default=False, action="store_true")
parser.add_argument("-t", help="time in seconds to test (default 10 secs)", default=10, type=int)
args = parser.parse_args()

os.environ["BRIDGE"]="ykk-ovs-test"

def debug(string):
    print '\033[1;32;40m';
    print string + '\033[0m'

def ovs_setting():
    debug('Network topology')

    ret = os.system("./ovs_setup.sh")
    if ret != 0:
        print '\033[1;31;40mOVS settting interrupted, please run again. \033[0m'
        return -1

    # print some network setting info
    if args.v == True:
        debug("Network setting")
        os.system("ip netns exec ns1 ifconfig -a tap1")
        os.system("ip netns exec ns1 route -n")
        os.system("ip netns exec ns2 ifconfig -a tap2")
        os.system("ip netns exec ns2 route -n")

    return 0

def ping_test():
    # run the ping test
    debug("Ping test between \"tap1\" and \"tap2\"")
    ret=os.system("ip netns exec ns1 ping -c 2 10.1.1.5")
    if ret != 0:
        print '\033[1;31;40mPING test failed, please check your OVS setting. \033[0m'
        return -1
    return 0

def udp_test():
    # run udp test on background
    os.system("ip netns exec ns1 ./cstest/run.sh -s &")
    os.system("ip netns exec ns2 ./cstest/run.sh -c &> /dev/null &")

def kill_childs():
    os.system("ps -aux|grep cs | grep udp | awk '{print $2}' | xargs -i kill {}")


# listen data flow
total_pps = 0
prev_total_pps = 0
total_pps_cnt = 0

def get_flow():
    global total_pps
    global prev_total_pps

    total_pps = 0
    ss = os.popen("ovs-ofctl dump-flows $BRIDGE | sed -n '/tp_dst/p' | "
                  "awk '{print $4}' | sed 's/[^0-9]//g'").readlines()
    for s in ss:
        port_pps = int(s)
        total_pps += port_pps

    current_pps = total_pps - prev_total_pps
    prev_total_pps = total_pps
    return current_pps

def listen_flow():
    global total_pps_cnt

    debug("Start to listen the pps of OVS")
    time.sleep(1)
    start = time.time()
    while True:
        pps_start = time.clock()

        cur_pps = get_flow()
        total_pps_cnt += 1

        print "OVS's pps (1024 byte/packet) is %f Mpps"%(cur_pps / 1024.0 / 1024.0)
        print "OVS's pps (16 byte/packet)   is %f Mpps"%(cur_pps * 16 / 1024.0 / 1024.0)
        print "--------------------------------------------"

        while ((time.clock() - pps_start) < 1.0005):
            pass

        end = time.time()
        if int(end - start) > args.t:
            return 0;

if __name__ == "__main__":
    try:
        ret = ovs_setting()
        if ret != 0:
            raise SystemExit, 0

        ret = ping_test()
        if ret != 0:
            raise SystemExit, 0

        udp_test()
        listen_flow()

    except SystemExit:
        sys.exit(1)
    except KeyboardInterrupt:
        pass

    if prev_total_pps != 0:
        debug("Average values")
        print "Average pps (1024 byte/packet) is %f Mpps"%(total_pps / total_pps_cnt / 1024.0 / 1024.0)

    kill_childs()
