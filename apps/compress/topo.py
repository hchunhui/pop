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
        h1 = self.addHost('h1')
        h2 = self.addHost('h2')
        h3 = self.addHost('h3')

        # Add links
        self.addLink(h1, s1, 1, 1)
        self.addLink(s1, s2, 2, 1)
        self.addLink(s2, s3, 2, 1)
        self.addLink(s3, h3, 2, 1)
        self.addLink(h2, s2, 1, 3)

topos = { 'mytopo': ( lambda: MyTopo() ) }
