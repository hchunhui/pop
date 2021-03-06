"""Custom topology example

Two directly connected switches plus a host for each switch:

   host --- switch --- switch --- host

Adding the 'topos' dict with a key/value pair to generate our newly defined
topology enables one to pass in '--topo=mytopo' from the command line.
"""

from mininet.topo import Topo

class MyTopo( Topo ):
    "Simple topology example."

    def __init__( self ):
        "Create custom topo."

        # Initialize topology
        Topo.__init__( self )

        # Add hosts and switches
        s1 = self.addSwitch('s1')
        s2 = self.addSwitch('s2')
        s3 = self.addSwitch('s3')
        s4 = self.addSwitch('s4')
        h1 = self.addHost('h1')
        h2 = self.addHost('h2')
        h3 = self.addHost('h3')
        h4 = self.addHost('h4')

        # Add links
        self.addLink(h1, s1, 1, 1)
        self.addLink(h2, s2, 1, 1)
        self.addLink(h3, s3, 1, 1)
        self.addLink(h4, s4, 1, 1)
        self.addLink(s1, s2, 2, 2)
        self.addLink(s2, s3, 3, 3)
        self.addLink(s3, s4, 2, 2)
        self.addLink(s4, s1, 3, 3)
        self.addLink(s1, s3, 4, 4)

topos = { 'mytopo': ( lambda: MyTopo() ) }
