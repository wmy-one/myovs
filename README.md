Network Topology:
--------
    ------------------                             ------------------
    |                |                             |                |
    |         -------|Veth Pair           Veth Pair|-------         |
    |         | tap1 |------                -------| tap2 |         |
    |         -------|     |                |      |-------         |
    |                |     |                |      |                |
    |Namespace "ns1" |     |                |      | Namespace "ns2"|
    ------------------     |                |      ------------------
                           v                v
                       ---------------------------
                       |ovs-tap1         ovs-tap2|
                       |                         |
                       |       Openvswith        |
                       ---------------------------


TODO
-----
- Need to add the flow table

References:
------

[基于 Open vSwitch 的 OpenFlow 实践](1)

[使用open vswitch构建虚拟网络](2)

[Linux Switching – Interconnecting Namespaces](3)

[How to Connect network namespaces using OpenvSwitch and veth pairs](4)

[UDP made simple](5)

[udp&tcp 对 epoll 的共用](6)

[6]: http://blog.csdn.net/lipengfei634626165/article/details/8136715
[5]: https://www.abc.se/~m6695/udp.html
[4]: http://fosshelp.blogspot.com/2014/10/network-namespaces-openvswitch-veth.html
[3]: http://www.opencloudblog.com/?p=66
[2]: http://blog.chinaunix.net/uid-20737871-id-4333314.html
[1]: http://www.ibm.com/developerworks/cn/cloud/library/1401_zhaoyi_openswitch/
