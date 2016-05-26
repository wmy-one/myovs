#! /usr/bin/python
# Author: yakir.yang@qq.com

import argparse
import commands
import os
import re
import signal
import sys

if os.geteuid() != 0:
    print '\033[1;31;40mThis program must be run as root. Aborting.\033[0m'
    sys.exit(1)

def signal_handler(signal, frame):
    print('You pressed Ctrl+C!')
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

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
    os.system("./ovs_setup.sh")

    # print some network setting info
    if args.v == True:
        debug("Network setting")
        os.system("ip netns exec ns1 ifconfig -a tap1")
        os.system("ip netns exec ns1 route -n")
        os.system("ip netns exec ns2 ifconfig -a tap2")
        os.system("ip netns exec ns2 route -n")

def ping_test():
    # run the ping test
    debug("Ping test between \"tap1\" and \"tap2\"")
    ret=os.system("ip netns exec ns1 ping -c 2 10.1.1.5")
    if ret != 0:
        print '\033[1;31;40m PING test failed, please check your OVS setting. \033[0m'
        return -1
    return 0

def udp_test():
    # run udp test on background
    os.system("ip netns exec ns1 ./cstest/run.sh -s &")
    os.system("ip netns exec ns2 ./cstest/run.sh -c &")

def kill_childs():
    os.system("ps -aux|grep run|grep cs | awk '{print $2}' | xargs -i kill {}")

def extractData(regex, content, index=1):
    r = '0'
    p = re.compile(regex)
    m = p.search(content)
    if m:
        r = m.group(index)
    return r


def get_flow():
    ss = os.popen("ovs-ofctl dump-flows $BRIDGE").readlines()
    for s in ss:
        print s
	print extractData(ur'n_packets=', s, 0)

if __name__ == "__main__":
    try:
        #ovs_setting()
        #ret = ping_test()
        #if ret != 0:
        #    raise SystemExit, 0
        #udp_test()
        get_flow()
    except SystemExit:
        sys.exit(1)
    finally:
        kill_childs()
