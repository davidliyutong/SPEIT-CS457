# How to run
To run the code, `mininet` must be configured correctly. See [Mininet Install Instruction](http://mininet.org/download/) for details.

Once correctly configured, run this command in terminal to perform a simulation:

```bash
sudo python3 questions.py
```
The results are directly printed in the terminal window

# Interpretation

## Question1
After we started the network, `net.iperf()` was used to test TCP bandwidth between hosts. The results are:
| Host - pair | Server Rate    | Client Rate    |
| ----------- | -------------- | -------------- |
| h1 <-> h2   | 9.56 Mbits/sec | 9.91 Mbits/sec |
| h2 <-> h3   | 9.57 Mbits/sec | 9.69 Mbits/sec |
| h3 <-> h1   | 9.57 Mbits/sec | 9.69 Mbits/sec |

That is satisfying for CPU limited hosts.

## Question2
We add a 5% package loss rate to link (s1, s2), (s1, s3) and test TCP bandwidth between hosts. The results are:
| Host - pair | Server Rate    | Client Rate    |
| ----------- | -------------- | -------------- |
| h1 <-> h2   | 6.30 Mbits/sec | 6.44 Mbits/sec |
| h2 <-> h3   | 1.85 Mbits/sec | 1.88 Mbits/sec |
| h3 <-> h1   | 7.61 Mbits/sec | 7.74 Mbits/sec |

All links are downgraded greatly. A 5% loss rate can have a huge impact on TCP connections: when a package is dropped, the TCP connection get interrupted. The transmitting party will then try to reconnect to the sender, wasting a great amount of bandwidth. In particular, the TCP bandwidth between `h2` and `h3` is much slower than other links. This is because their connection are interrupted by two downgraded links.

## Question3
After the additional link (s2, s3) was added, we use `net.ping()` to test the connectivity between hosts. The results are:
```
h1 -> X X
h2 -> X X
h3 -> X X
*** Results: 100% dropped (0/6 received)
```

This is because the three switch `h1`, `h2` and `h3` formulated a loop network, yet nothing was configured to solve the conflict. (like Spanning Tree Protocol)

## Solution
To solve the conflict in loop network. We add custom flow rules to the switches:
```python
print("Adding smart ovs flow rules")

quietRun('sudo ovs-ofctl add-flow s1 dl_dst={},actions=output:1'.format(net.getNodeByName('h1').MAC()))
quietRun('sudo ovs-ofctl add-flow s1 dl_dst={},actions=output:2'.format(net.getNodeByName('h2').MAC()))
quietRun('sudo ovs-ofctl add-flow s1 dl_dst={},actions=output:3'.format(net.getNodeByName('h3').MAC()))
quietRun('sudo ovs-ofctl add-flow s2 dl_dst={},actions=output:1'.format(net.getNodeByName('h1').MAC()))
quietRun('sudo ovs-ofctl add-flow s2 dl_dst={},actions=output:2'.format(net.getNodeByName('h2').MAC()))
 quietRun('sudo ovs-ofctl add-flow s2 dl_dst={},actions=output:3'.format(net.getNodeByName('h3').MAC()))
 quietRun('sudo ovs-ofctl add-flow s3 dl_dst={},actions=output:1'.format(net.getNodeByName('h1').MAC()))
quietRun('sudo ovs-ofctl add-flow s3 dl_dst={},actions=output:3'.format(net.getNodeByName('h2').MAC()))
quietRun('sudo ovs-ofctl add-flow s3 dl_dst={},actions=output:2'.format(net.getNodeByName('h3').MAC()))

```

After the installation of new flow rules, the switches are able to identify packages by their destinations. Switches then send packages to their desired destinations. All hosts can `ping` each other:

```
h1 -> h2 h3
h2 -> h1 h3
h3 -> h1 h2
*** Results: 0% dropped (6/6 received)
```
