#! /usr/bin/python
# Author: yakir.yang@qq.com
# Version: v1.1

import argparse
import commands
import os
import re
import signal
import sys
import time

os.chdir(os.path.dirname(os.path.abspath(__file__)))

if os.geteuid() != 0:
    print '\033[1;31;40mThis program must be run as root. Aborting.\033[0m'
    sys.exit(1)

parser = argparse.ArgumentParser(prog='PROG')
parser = argparse.ArgumentParser(description="An experiment about openvswitch.")
parser.add_argument("-v", help="print some debug messages and keep the OVS setting alive",
                    default=False, action="store_true")
parser.add_argument("-t", help="time in seconds to UDP test (default 10 secs)", default=10, type=int)
args = parser.parse_args()

# set up the ovs parameters
os.environ["BRIDGE"]="ykk-ovs-test"
os.environ["SERVER_IP"]="10.1.1.4"
os.environ["CLIENT_IP"]="10.1.1.5"
os.environ["SERVER_SPORT"]="6783"
os.environ["TEST_PORTS"]="100"

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
    ret=os.system("ip netns exec ns1 ping -c 4 ${CLIENT_IP}")
    if ret != 0:
        print '\033[1;31;40mPING test failed, please check your OVS setting. \033[0m'
        return -1
    return 0

def udp_test_byte():
    # run udp test on background
    os.system("ip netns exec ns1 ./cstest/run.sh -s -spn $TEST_PORTS -pbs $PERBUF_SIZE -sport $SERVER_SPORT &")
    os.system("ip netns exec ns2 ./cstest/run.sh -c -spn $TEST_PORTS -pbs $PERBUF_SIZE -sport $SERVER_SPORT &")

    # wait for test timeout
    start = time.time();
    while time.time() - start < args.t:
        time.sleep(1)

    # kill the unused backgroud test
    os.system("ps -aux|grep cs | grep udp | awk '{print $2}' | xargs -i kill {}")

def udp_test_1024byte():
    debug("Start to run UDP test (1024 byte/packet)")
    os.environ["PERBUF_SIZE"]="1024"
    udp_test_byte()

def udp_test_64byte():
    debug("Start to run UDP test (64 byte/packet)")
    os.environ["PERBUF_SIZE"]="64"
    udp_test_byte()

def rollback_netenv():
    os.system("ps -aux|grep cs | grep udp | awk '{print $2}' | xargs -i kill {}")
    if args.v != True:
        os.system("bash ovs_destroy.sh")

if __name__ == "__main__":
    try:
        ret = ovs_setting()
        if ret != 0:
            raise SystemExit, 0

        ret = ping_test()
        if ret != 0:
            raise SystemExit, 0

        udp_test_1024byte()

        udp_test_64byte()

    except SystemExit:
        pass
    except KeyboardInterrupt:
        pass

    rollback_netenv()
