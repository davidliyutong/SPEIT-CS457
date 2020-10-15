from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import Host
from mininet.link import TCLink
from mininet.util import dumpNodeConnections
from mininet.log import setLogLevel, info
from mininet.util import quietRun


class MyTopo(Topo):
    def build(self, packet_loss=0, additional_link=False):
        # Add hosts and switches
        Host1 = self.addHost('h1')
        Host2 = self.addHost('h2')
        Host3 = self.addHost('h3')

        Switch1 = self.addSwitch('s1')
        Switch2 = self.addSwitch('s2')
        Switch3 = self.addSwitch('s3')

        # Add links
        self.addLink(Host1, Switch1)
        self.addLink(Switch1, Switch3, bw=10, loss=packet_loss)
        self.addLink(Switch3, Host3)

        self.addLink(Switch1, Switch2, bw=10, loss=packet_loss)
        self.addLink(Switch2, Host2)

        if additional_link:
            self.addLink(Switch2, Switch3)


def perf_test(topo, algorithm=None):
    net = Mininet(topo=topo, host=Host, link=TCLink, autoStaticArp=True)
    net.start()
    if algorithm is not None:
        algorithm(net)

    info("Dumping host connections\n")
    dumpNodeConnections(net.hosts)
    dumpNodeConnections(net.switches)
    info("Testing bandwidth between hosts\n")
    net.iperf((net.getNodeByName('h1'), net.getNodeByName('h2')), l4Type='TCP', seconds=10)
    net.iperf((net.getNodeByName('h2'), net.getNodeByName('h3')), l4Type='TCP', seconds=10)
    net.iperf((net.getNodeByName('h3'), net.getNodeByName('h1')), l4Type='TCP', seconds=10)
    net.stop()


def ping_test(topo, algorithm=None):
    net = Mininet(topo=topo, host=Host, link=TCLink, autoStaticArp=True)
    net.start()
    # net.ping((net.getNodeByName('h1'), net.getNodeByName('h2'), net.getNodeByName('h3')))
    if algorithm is not None:
        algorithm(net)

    info("Dumping host connections\n")
    dumpNodeConnections(net.hosts)
    dumpNodeConnections(net.switches)
    info("Testing ping\n")

    net.ping((net.getNodeByName('h1'), net.getNodeByName('h2'), net.getNodeByName('h3')), timeout=10)
    net.ping((net.getNodeByName('h1'), net.getNodeByName('h2'), net.getNodeByName('h3')), timeout=10)
    net.stop()


def custom_flow_control(net):
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



if __name__ == '__main__':
    setLogLevel('info')
    quietRun('sudo mn -c')
    packet_loss = 5

    # question 1
    print("\n#------------------ Question1 ------------------#\n\n")
    topo = MyTopo()
    perf_test(topo)
    print("\n\n#-----------------------------------------------#\n")

    # question 2
    print("\n#------------------ Question2 ------------------#\n\n")

    topo = MyTopo(packet_loss=packet_loss)
    perf_test(topo)
    print("\n\n#-----------------------------------------------#\n")

    # question 3
    print("\n#------------------ Question3 ------------------#\n\n")
    additional_link = True
    topo = MyTopo(packet_loss=packet_loss, additional_link=additional_link)
    ping_test(topo)
    print("\n\n#-----------------------------------------------#\n")

    # solution: add flow rules
    print("\n#------------------ Solution ------------------#\n\n")
    additional_link = True
    topo = MyTopo(packet_loss=packet_loss, additional_link=additional_link)
    ping_test(topo, algorithm=custom_flow_control)
    print("\n\n#-----------------------------------------------#\n")
