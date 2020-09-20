from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import CPULimitedHost
from mininet.link import TCLink
from mininet.util import dumpNodeConnections
from mininet.log import setLogLevel, info


class MyTopo1(Topo):
    def __init__(self):
        # Initialize topology
        Topo.__init__(self)

        # Add hosts and switches
        Host1 = self.addHost('h1')
        Host2 = self.addHost('h2')
        Host3 = self.addHost('h3')

        Switch1 = self.addSwitch('s1')
        Switch2 = self.addSwitch('s2')
        Switch3 = self.addSwitch('s3')

        # Add links
        self.addLink(Host1, Switch1)
        self.addLink(Switch1, Switch2)
        self.addLink(Switch2, Host2)
        self.addLink(Switch1, Switch3)
        self.addLink(Switch3, Host3)


class MyTopo2(Topo):
    def __init__(self):
        # Initialize topology
        Topo.__init__(self)

        # Add hosts and switches
        Host1 = self.addHost('h1')
        Host2 = self.addHost('h2')

        Switch1 = self.addSwitch('s1')
        Switch2 = self.addSwitch('s2')
        Switch3 = self.addSwitch('s3')
        Switch4 = self.addSwitch('s4')

        # Add links
        self.addLink(Host1, Switch1)

        self.addLink(Switch1, Switch3)
        self.addLink(Switch2, Switch3)
        self.addLink(Switch1, Switch4)
        self.addLink(Switch2, Switch4)

        self.addLink(Switch2, Host2)


topos = {'no-loop-net': (lambda: MyTopo1()), 'loop-net': (lambda: MyTopo2())}
