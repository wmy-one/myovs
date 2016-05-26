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

[Openflow 90 minutes](7)

[openflow-spec-v1.1.0.pdf](8)

[Openv Switch 完全使用手册](9)

[Comparing sFlow and NetFlow in a vSwitch](10)


[10]: http://www.cnblogs.com/popsuper1982/p/3800582.html
[9]: http://sdnhub.cn/index.php/openv-switch-full-guide/
[8]: http://archive.openflow.org/documents/openflow-spec-v1.1.0.pdf
[7]: https://www.nanog.org/meetings/nanog57/presentations/Monday/mon.tutorial.SmallWallace.OpenFlow.24.pdf
[6]: http://blog.csdn.net/lipengfei634626165/article/details/8136715
[5]: https://www.abc.se/~m6695/udp.html
[4]: http://fosshelp.blogspot.com/2014/10/network-namespaces-openvswitch-veth.html
[3]: http://www.opencloudblog.com/?p=66
[2]: http://blog.chinaunix.net/uid-20737871-id-4333314.html
[1]: http://www.ibm.com/developerworks/cn/cloud/library/1401_zhaoyi_openswitch/
