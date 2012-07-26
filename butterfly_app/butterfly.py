# Copyright 2012 (C) Budapest University of Technology and Economics
#
# This file is NOT part of Mininet.
#
# butterfly.py is free software: you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with NOX.  If not, see <http://www.gnu.org/licenses/>.
#
#
# Author: Felician Nemeth <nemethf@tmit.bme.hu>

"""Classical butterfly topology
"""

import inspect
import re
import csv
from time import sleep
from glob import glob
from os import system, path
from mininet.cli import CLI
from mininet.log import info, output, error
from mininet.net import Mininet
from mininet.util import quietRun
from mininet.topo import Topo, Node
from mininet.node import NOX, UserSwitch

def get_file_path():
    "Return the path of the this file."
    return path.dirname(inspect.getfile(inspect.currentframe()))

def get_topo_filename():
    return get_file_path() + '/bloom_ids.csv' 

def get_coords_filename():
    return get_file_path() + '/greedy_coords.csv'

def do_bottleneck( self, line ):
    """Set the link capacity of the bottleneck link (s7-eth3).
    Usage: bottleneck speed.  'speed' is measurend in mbit/s.
    Example: bottleneck 2.5"""

    args = line.split()
    if len(args) != 1:
        error( 'invalid number of args: bottleck speed\n' )
    else:
        speed = args[ 0 ]
        cmd = 'tc qdisc replace dev s7-eth3 root'
        cmd += ' tbf rate %smbit burst 5kb latency 70' % speed
        node = self.nodemap[ 's7' ]
        node.cmd( cmd )

def vlc_start_stream( self, host, filename, target, port ):
    "Start a VLC server at 'host' streaming 'filename' to 'port' of 'target'."
    try:
        ip = target.defaultIP
    except AttributeError:
        ip = target
    title = '%s:%s' % (host.name, port)
    pidfile = '/tmp/mininet_vlc_%s_%s' % (host.name, port)
    cmd = 'sudo -u openflow'
    cmd += ' cvlc -d -q --loop "%s"' % filename
    cmd += ' --pidfile %s' % pidfile
    cmd += ' :sout=#udp{dst=%s:%s} ' % ( ip, port )
    cmd += ' :no-sout-rtp-sap :no-sout-standard-sap :ttl=10 :sout-keep'
    if self.mn.controller_name in ['mpls', 'nc']:
        host.cmd( 'arp -s %s ff:ff:ff:ff:ff:ff' % ip )
    host.cmd( 'ip link set mtu 1480 dev %s-eth0' % host.name )
    host.cmd( cmd + ' &' )

def vlc_start_client( self, host, port, x, y ):
    "Start a VLC client at 'host' listening at 'port' and move it to 'x','y'."
    title = '%s:%s' % (host.name, port)
    pidfile = '/tmp/mininet_vlc_%s_%s' % (host.name, port)
    cmd = 'sudo -u openflow'
    cmd += ' cvlc -d -q --video-title "%s:%s"' % (host.name, port)
    cmd += ' --pidfile %s' % pidfile
    cmd += ' --video-x %s --video-y %s' % (x, y)
    cmd += ' udp://@:%s' % port
    host.cmd( cmd + ' &' )

def vlc_start_vlc( self ):
    "Start VLC programs."
    port_1 = 1111
    port_2 = 2222
    movie_1 = get_file_path() + '/movies/b.avi'
    movie_2 = get_file_path() + '/movies/c.avi'
    for f in [movie_1, movie_2]:
        if not path.exists(f):
            info( 'File does not exists: %s\n' % f )
            return
    h1 = self.mn.hosts[ 0 ]
    h2 = self.mn.hosts[ 1 ]
    h3 = self.mn.hosts[ 2 ]
    h4 = self.mn.hosts[ 3 ]
    if self.mn.controller_name == 'bloom':
        self.vlc_start_stream( h1, movie_1, '10.0.3.4', port_1 )
    elif self.mn.controller_name == 'greedy':
        self.vlc_start_stream( h1, movie_1, '10.0.0.4', port_1 )
    else:
        self.vlc_start_stream( h1, movie_1, h3, port_1 )
        self.vlc_start_stream( h2, movie_2, h4, port_2 )
    self.vlc_start_client( h3, port_1, 400, 500 )
    self.vlc_start_client( h3, port_2, 400, 600 )
    self.vlc_start_client( h4, port_1, 650, 500 )
    self.vlc_start_client( h4, port_2, 650, 600 )
    if self.mn.controller_name == 'bloom':
        self.vlc_start_client( h2, port_1, 400, 500 )
        self.vlc_start_client( h2, port_2, 400, 600 )

def do_vlc( self, line ):
    """Start VLC video streams at h1 and h3, and VLC receivers at h3, h4.
    Usage: vlc [start|stop|restart]"""

    args = line.split()
    if len(args) == 0:
        self.vlc_start_vlc()
        return
    elif len(args) > 1:
        error( 'invalid number of args: see help vlc\n' )
        return
    elif args[0] == 'start':
        self.vlc_start_vlc()
        return 
    elif args[0] == 'stop':
        self.vlc_stop_vlc()
        return 
    elif args[0] == 'restart':
        self.vlc_stop_vlc()
        self.vlc_start_vlc()
        return
    else:
        error( 'unknown command: see help vlc\n' )

def complete_vlc(self, text, line, begidx, endidx):
    a = [ 'start', 'stop', 'restart' ]
    return [i for i in a if i.startswith(text)]

def vlc_stop_vlc( self ):
    "Stop vlc programs."
    for filename in glob( '/tmp/mininet_vlc_*' ):
        self.kill_from_pid_file( filename )

def kill_from_pid_file( self, filename ):
    "Kill the program whose pid is stored in `filename'."
    from os import kill, unlink
    from glob import glob
    from signal import SIGKILL

    try:
        file = open( filename, 'r' )
    except IOError:
        return
    pid = file.read(100)
    kill( int(pid), SIGKILL )
    unlink( filename )

def my_stop_all( self ):
    "Stop the traffic monitor, vlc programs and then stop the nodes."
    self.kill_from_pid_file( '/tmp/traffic_monitor.pid' )
    self.vlc_stop_vlc()
    self.my_stop_orig()

CLI.do_bottleneck = do_bottleneck
CLI.vlc_start_stream = vlc_start_stream
CLI.vlc_start_client = vlc_start_client
CLI.vlc_start_vlc = vlc_start_vlc
CLI.vlc_stop_vlc = vlc_stop_vlc
CLI.kill_from_pid_file = kill_from_pid_file
CLI.do_vlc = do_vlc
CLI.complete_vlc = complete_vlc
Mininet.kill_from_pid_file = kill_from_pid_file
Mininet.vlc_stop_vlc = vlc_stop_vlc
Mininet.my_stop_orig = Mininet.stop
Mininet.stop = my_stop_all

class BloomHelper():
    """Helper class for Bloom filer functionality."""

    def __init__( self, mn ):
        self.mn = mn

    def set_mac( self, nodename, mac ):
        "Set the mac address of the node's first interface."
        node = self.mn.nameToNode[ nodename ]
        node.MAC()
        mac_info = node.macs.items()[ 0 ]
        if_name = mac_info[ 0 ]
        node.macs[ if_name ] = mac

        node.cmd( "ip link set dev %s down" % if_name )
        node.cmd( "ip link set dev %s address %s " % (if_name, mac) )
        node.cmd( "ip link set dev %s up" % if_name )

    def get_id( self, node_from, node_to ):
        try:
            src = re.sub( '[hs]', '', node_from )
            src = int( src )
            dst = re.sub( '[hs]', '', node_to )
            dst = int( dst )
        except ValueError:
            info( 'error: bloom_get_id: %s, %s' % ( node_from, node_to ) )
            src = dst = 0
        try:
            return self.mn.topo.edges_db[ src ][ dst ]
        except KeyError:
            info( ' unkown link %s -> %s \n' % ( node_from, node_to ) )
            return "000000000000"

    def mac_addr_to_hex( self, addr ):
        h = addr.replace( ':', '' )
        return int( h, 16 )

    def format_mac( self, addr_as_int ):
        s = '%.12x' % addr_as_int
        s = re.sub( '(..)(..)(..)(..)(..)(..)', '\\1:\\2:\\3:\\4:\\5:\\6', s )
        return s

    def get_mac_from_route( self, route, print_info = False ):
        mac = 0x0;
        for r in route:
            for node_from, node_to in zip( r[:-1], r[1:] ):
                bloom_id = self.get_id( node_from, node_to )
                bloom_id = int( bloom_id, 16 )
                mac = mac | bloom_id
                if print_info:
                    info( '{:3}->{:3}: {:0>48b}\n'.format( node_from, node_to, bloom_id ) )
        if print_info:
            info( '{:>9} {:0>48b}\n'.format( '=====>', mac ) )
            info( '{:>9} {:s}\n'.format( '=====>', self.format_mac( mac ) ) )
        return self.format_mac( mac )

def do_controller( self, line ):
    """(Re)start the controller in Network Coding or in MPLS multicast mode.
    Usage: controller {nc|mpls|...}"""

    def get_greedy_mac( node_id ):
        [x, y] =  self.mn.topo.coords[ node_id ]
        addr = '%.6x%.6x' % ( x, y )
        addr = re.sub( '(..)(..)(..)(..)(..)(..)',
                       '\\3:\\2:\\1:\\6:\\5:\\4', addr )
        return addr

    def is_switch( node_id ):
        ports = self.mn.topo.edges_db[ node_id ]
        return ( len(ports) > 1 )

    args = line.split()
    if ( len(args) != 1 ) or ( args[0] not in controllers.keys() ):
        error( 'Error: see help controller\n' )
        return

    self.mn.controller = controllers[ args[0] ]
    self.mn.controller_name = args[0]
    info( '*** Stopping %i controllers\n' % len( self.mn.controllers ) )
    for controller in self.mn.controllers:
        controller.stop()
    self.mn.controllers = []
    info( '*** Starting controller\n' )
    self.mn.addController( 'c0' )
    for controller in self.mn.controllers:
        controller.start()

    if args[0] == 'greedy':
        info( '*** Intializing nodes\n' )
        for node_id in self.mn.topo.edges_db:
            if is_switch( node_id ):
                continue
            node_name = 'h%d' % node_id
            info( node_name + ' ')
            ip_addr = '10.0.0.%d' % node_id;
            if_name = '%s-eth0' % node_name
            mac_addr = get_greedy_mac( node_id )

            node = self.nodemap[ node_name ]
            node.cmd( "ip link set dev %s down" % if_name )
            node.cmd( "ip link set dev %s address %s" % (if_name, mac_addr) )
            node.cmd( "ip link set dev %s up" % if_name )
            for node_id_2 in self.mn.topo.edges_db:
                if is_switch( node_id_2 ):
                    continue
                ip_addr_2 = '10.0.0.%d' % node_id_2;
                mac_addr_2 = get_greedy_mac( node_id_2 )
                node.cmd( "arp -s %s %s " % (ip_addr_2, mac_addr_2) )
        info( '\n' )
    if args[0] == 'bloom':
        info( '*** Intializing nodes\n' )
        self.bloom = BloomHelper( self.mn )
        mac = {}
        for node, ports in self.mn.topo.edges_db.iteritems():
            if len(ports) == 1:
                name = 'h%d' % node
                addr = ports.values()[ 0 ]
                addr = int( addr, 16 )
                addr = self.bloom.format_mac( addr )
                mac[ name ] = addr
        for name, addr in mac.iteritems():
            info( ' %s' % name )
            self.bloom.set_mac( name, addr )
        info( '\n*** Intializing routes\n' )
        routes = [
            [ 'h1', '10.0.0.3', ['s5', 's9', 'h3'] ],
            [ 'h1', '10.0.0.4', ['s5', 's7', 's8', 's10', 'h4'] ],
            [ 'h1', '10.0.3.4', ['s5', 's9', 'h3'], [ 's5', 's7', 's8', 's10', 'h4' ] ],
            [ 'h3', '10.0.0.1', ['s9', 's5', 'h1'] ],    
            [ 'h2', '10.0.0.3', ['s6', 's10', 'h4'] ],
            [ 'h2', '10.0.0.4', ['s6', 's7', 's8', 's9', 'h3'] ],
            [ 'h2', '10.0.3.4', ['s6', 's10', 'h4'], ['s6', 's7', 's8', 's9', 'h3'] ],
            [ 'h4', '10.0.0.1', ['s10', 's8', 's7', 's5', 'h1'] ] ]
        for r in routes:
            src = r[ 0 ]
            ip_addr = r[ 1 ]
            route = r[ 2: ]
            info( ' %s:%s' % ( src, ip_addr ) )
            node = self.mn.nameToNode[ src ]
            mac_addr = self.bloom.get_mac_from_route( route )
            node.cmd( 'arp -s %s %s' % ( ip_addr, mac_addr ) )
        info( '\n' )
    if args[0] == 'greedy':
        system( 'touch /tmp/topo_greedy' )
    else:
        system( 'rm -f /tmp/topo_greedy' )
        

def complete_controller(self, text, line, begidx, endidx):
    return [i for i in controllers.keys() if i.startswith(text)]

CLI.do_controller = do_controller
CLI.complete_controller = complete_controller

def do_topo_ascii_art( self, line ):
    "Print the ascii art representation of the network topology."
    str = r"""        /----\                /----\
        | h1 |                | h2 |
        \-+--/                \-+--/
          |                     |
        +-+--+                +-+--+
        | s5 |                | s6 |
        +-+--+                +-+--+
          |   \---\      /---/  |
          |        +----+       |
          |        | s7 |       |
          |        +-+--+       |
          |          |          |
          |        +-+--+       |
          |        | s8 |       |
          |        +----+       |
          |   /---/      \---\  |
        +-+--+                +-+---+
        | s9 |                | s10 |
        +-+--+                +-+---+
          |                     |
        /-+--\                /-+--\
        | h3 |                | h4 |
        \----/                \----/
"""
    info( str )

CLI.do_topo_ascii_art = do_topo_ascii_art

def do_set_bloom_route( self, line ):
    """Installs an ARP entry at Host. Mac address is calculated from
links Bloom-id. Syntax: Host ip_address node-node-... ...
Example: set_bloom_route h1 10.0.3.4 s5-s9-h3 s5-s7-s8-s10-h4"""

    r = line.split()
    if len(r) < 3:
        error( 'invalid number of args: see help set_bloom_route\n' )
        return
    src = r[ 0 ]
    ip_addr = r[ 1 ]
    route = r[ 2: ]
    route = [r.split("-") for r in route]
    info( ' %s:%s\n' % ( src, ip_addr ) )
    try:
        node = self.mn.nameToNode[ src ]
    except KeyError:
        error( 'unknown host: %s\n' % src )
        return
    mac_addr = self.bloom.get_mac_from_route( route, True )
    node.cmd( 'arp -s %s %s' % ( ip_addr, mac_addr ) )
    info( '\n' )
CLI.do_set_bloom_route = do_set_bloom_route

def my_start( self ):
    "Start normally, then start the traffic monitor GUI."

    self.my_start_orig()
    self.controller_name = 'mpls'
    d_path = path.dirname(inspect.getfile(inspect.currentframe()))
    system( 'perl %s/topo.pl &' % d_path )

Mininet.my_start_orig = Mininet.start
Mininet.start = my_start

class ButterflyTopo( Topo ):
    """The classic butterfly topology.  See do_topo_ascii_art().
    Command line option: --topo=butterfly"""

    def __init__( self, enable_all = True ):
        "Create the topo."

        # Add default members to class.
        super( ButterflyTopo, self ).__init__()

        # Load topology from file
        f = open( get_topo_filename() )
        reader = csv.reader( f, delimiter=',' )
        self.edges_db = {}
        for r in reader:
            node_id     = int( r[ 0 ] )
            neighbor_id = int( r[ 1 ] )
            port_no     = int( r[ 2 ] ) # not needed here
            addr        = r[ 3 ].strip()
            try:
                self.edges_db[ node_id ][ neighbor_id ] = addr
            except KeyError:
                self.edges_db[ node_id ] = { neighbor_id: addr }

        # Add nodes
        nodes = self.edges_db.keys()
        nodes.sort()
        for node in nodes:
            ports = self.edges_db[ node ]
            is_switch = ( len(ports) > 1 )
            self.add_node( node, Node( is_switch=is_switch ) )

        # Add edges
        for node in nodes:
            neighbors = self.edges_db[ node ].keys()
            neighbors.sort()
            for neighbor in neighbors:
                if node < neighbor:
                    self.add_edge( node, neighbor )

        # Consider all switches and hosts 'on'
        self.enable_all()

        # Load greedy coordinates from file
        f = open( get_coords_filename() )
        reader = csv.reader( f, delimiter=',' )
        self.coords = {}
        for r in reader:
            node_id = int( r[ 0 ] )
            x       = int( r[ 1 ] )
            y       = int( r[ 2 ] )
            self.coords[ node_id ] = [ x, y ]

# Changing the defaults by redefining 'minimal', 'ovsk', 'ref' is
# really ugly, but those are useless in our scenario.
#
bloom_command = "'butterfly app'=bloom,links=%s" % get_topo_filename()
greedy_command = "'butterfly app'=greedy,links=%s,coords=%s" \
    % ( get_topo_filename(), get_coords_filename() )

topos = { 'minimal': ( lambda: ButterflyTopo() ),
          'butterfly': ( lambda: ButterflyTopo() ) } 
switches = { 'ovsk': UserSwitch }
controllers = { 'ref': lambda name: NOX( name, "'butterfly app'" ),
                'nc' : lambda name: NOX( name, "'butterfly app'=nc" ),
                'mpls' : lambda name: NOX( name, "'butterfly app'" ),
                'bloom': lambda name: NOX( name, bloom_command ),
                'greedy' : lambda name: NOX( name, greedy_command ) }
