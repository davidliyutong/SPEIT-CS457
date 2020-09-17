## Semaine 1

### iperf
```
iperf [-s | serve][-c | client][-p | port][destination]
```

### Custom Topology
```
sudo mn --custom topo.py --topo task1
```

topo.py:

```python
from mininet.topo import Topo

class MyTopo( Topo ):

    def __init__( self ):

        # Initialize topology
        Topo.__init__( self )

        # Add hosts and switches
        Host1 = self.addHost( 'h1' )
        Host2 = self.addHost( 'h2' )
        Host3 = self.addHost( 'h3' )

        Switch1 = self.addSwitch( 's1' )
        Switch2 = self.addSwitch( 's2' )
        Switch3 = self.addSwitch( 's3' )

        # Add links
        self.addLink( Host1, Switch1 )
        self.addLink( Switch1, Switch2 )
        self.addLink( Switch2, Host2 )
        self.addLink( Switch1, Switch3 )
        self.addLink( Switch3, Host3 )

topos = { 'task1': ( lambda: MyTopo() ) }
```



