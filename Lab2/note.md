## Semaine 2

### 问题： ping 的延迟
```
mininet> h1 ping h2
PING 10.0.0.2 (10.0.0.2) 56(84) bytes of data.
64 bytes from 10.0.0.2: icmp_seq=1 ttl=64 time=0.431 ms
64 bytes from 10.0.0.2: icmp_seq=2 ttl=64 time=0.069 ms
64 bytes from 10.0.0.2: icmp_seq=3 ttl=64 time=0.077 ms
64 bytes from 10.0.0.2: icmp_seq=4 ttl=64 time=0.053 ms
64 bytes from 10.0.0.2: icmp_seq=5 ttl=64 time=0.065 ms
64 bytes from 10.0.0.2: icmp_seq=6 ttl=64 time=0.068 ms
```
**问：第一次的ping耗时比其他的ping耗时长，为什么？**

这是因为主机第一次ping会启动一个ARP（Address Resolution Protocol）广播，试图确定目标地址的物理地址。主机发送信息时将包含目标IP地址的ARP请求广播到局域网络上的所有主机，并接收返回消息，以此确定目标的物理地址；收到返回消息后将该IP地址和物理地址存入本机ARP缓存中并保留一定时间，下次请求时直接查询ARP缓存以节约资源。

**问题：为什么看不到host的网卡？**
这和mininet技术实现有关。mininet使用了虚拟机技术来模拟host，因此host的网卡无法被wireshark发现（wireshark只能发现宿主机的网卡）。同样的道理，交换机的网卡创建在宿主机，因此可以被mininet发现。

### 手动添加转发规则
```
sudo ovs-ofctl add-flow s1 "in_port=1 actions=output:2"
```

### 查看Switch已经存在的flow_rule
```
sudo ovs-ofctl dump-flows s1
```

### 查看交换机/交换机运行的协议
```
sudo ovs-vsctl show
```



