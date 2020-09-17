from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import CPULimitedHost
from mininet.link import TCLink
from mininet.util import dumpNodeConnections
from mininet.log import setLogLevel, info


class MyTopo(Topo):
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
		self.addLink(Switch1, Switch3)
		self.addLink(Switch2, Host2)
		self.addLink(Switch1, Switch3)
		self.addLink(Switch3, Host3)


def perfTest():
	topo = MyTopo()
	net = Mininet(topo=topo, host=CPULimitedHost, link=TCLink, autoStaticArp=True)
	net.start()
	info("Dumping host connections\n")
	dumpNodeConnections(net.hosts)
	info("Testing bandwidth between h1 and h3\n")
	h1, h3 = net.getNodeByName('h1', 'h3')
	net.iperf((h1, h3), l4Type='UDP')
	net.stop()


if __name__ == '__main__':
	setLogLevel('info')
	perfTest()
