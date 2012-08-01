#!/usr/bin/perl
#                                              -*- mode: cperl; -*-

# Copyright 2012 (C) Budapest University of Technology and Economics
#
# This file is NOT part of Mininet.
#
# topo.pl is free software: you can redistribute it and/or
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
# Author: Felicián Németh <nemethf@tmit.bme.hu>

use strict;
use warnings;
use Data::Dumper;
use Goo::Canvas;
use Gtk2 '-init';
use Glib qw(TRUE FALSE);

my $REMOTE = grep {/remote/} @ARGV;
my $SILENT = scalar (grep {/verbose/} @ARGV) == 0;

open PID, '>/tmp/traffic_monitor.pid';
print PID $$;
close PID;

my $LINE;
my $root;

my %nodes = ();

my %pos_nc = (
  'h1' => [1, 3],
  's5' => [2, 3],
  's7' => [3, 2],
  's8' => [4, 2],
  's9' => [5, 3],
  'h3' => [6, 3],
  'h2' => [1, 1],
  's6' => [2, 1],
  's10' => [5, 1],
  'h4' => [6, 1],
);

my %pos_greedy = (
  'h1' => [2, 3],
  's5' => [3, 3],
  's7' => [3, 2],
  's8' => [4, 2],
  's9' => [4, 3],
  'h3' => [5, 3],
  'h2' => [2, 1],
  's6' => [3, 1],
  's10' => [4, 1],
  'h4' => [5, 1],
);

my %ports = (
  's5' => ['h1', 's7', 's9'],
  's6' => ['h2', 's7', 's10'],
  's7' => ['s5', 's6', 's8'],
  's8' => ['s7', 's9', 's10'],
  's9' => ['h3', 's5', 's8'],
  's10' => ['h4', 's6', 's8'],
);

my %ip_addr = (
  '192.168.213.35' => 's5',
  '192.168.213.36' => 's6',
  '192.168.213.37' => 's7',
  '192.168.213.38' => 's8',
  '192.168.213.39' => 's9',
  '192.168.213.40' => 's10',
);

sub my_print {
  my ($str) = @_;
  return
    if $SILENT;

  print $str;
}

sub set_nodes {
  my ($pos) = @_;

  foreach my $node (keys %$pos) {
      $nodes{$node}[0] = -25 + 55 * $pos->{$node}[0];
      $nodes{$node}[1] = -25 + 55 * $pos->{$node}[1];
  }
}

my %links = ();
my $timer_link;
sub add_link {
  my ($root, $x1, $y1, $x2, $y2) = @_;

  my $line = Goo::Canvas::Polyline->new(
        $root, FALSE,
        [ $x1, $y1,
          $x2, $y2 ],
        'stroke-color' => 'midnightblue',
        'line-width' => 3,
        'start-arrow' => FALSE,
        'end-arrow' => FALSE,
        'arrow-tip-length' => 3,
        'arrow-length' => 4,
        'arrow-width' => 3.5
    );

  if (defined $timer_link) {
    Glib::Source->remove($timer_link);
    $timer_link = undef;
  }

  return $line;
}

sub get_link {
  my ($src, $dst) = @_;

  return $links{"$src,$dst"} || $links{"$dst,$src"};
}

sub remove_link {
  my ($root, $src, $dst) = @_;

  my $link = get_link($src, $dst);
  return
    unless defined $link;

  my $i = $root->find_child($link);
  $root->remove_child($i);

  delete $links{"$src,$dst"};
  delete $links{"$dst,$src"};
}

sub add_links {
  foreach my $node_src (keys %ports) {
    foreach my $node_dst (@{$ports{$node_src}}) {
      unless (get_link($node_src, $node_dst)) {
        my $x1 = $nodes{$node_src}[0];
	my $y1 = $nodes{$node_src}[1];
	my $x2 = $nodes{$node_dst}[0];
	my $y2 = $nodes{$node_dst}[1];

	my $link = add_link($root, $x1, $y1, $x2, $y2);
	$links{"$node_src,$node_dst"} = $link;
	$link->{diff} = 0;
	$link->lower;
      }
    }
  }
}

sub add_node {
  my ($root, $x, $y, $name) = @_;

  my $group = Goo::Canvas::Group->new($root);

  my $x1 = $x - 15;
  my $y1 = $y - 15;
  my $rect = Goo::Canvas::Rect->new(
    $group, $x1, $y1, 30, 30,
    'line-width' => 2,
    'radius-x' => 2,
    'radius-y' => 1,
    'stroke-color' => 'blue',
    'fill-color' => 'lightblue'
  );
  $rect->signal_connect('button-press-event',
			\&on_rect_button_press);

  my $text = Goo::Canvas::Text->new(
    $group, $name, $x, $y, -1, 'center',
    'font' => 'Sans 10',
  );

  $nodes{$name}[2] = $group;
}

sub get_stats {
  my @sw = glob '/tmp/s[0-9]*';
  my %db;
  my $cmd = 'stats-port';
  my $var = 'tx_pkt';

  my @list = grep {/tmp\/(s[0-9]+)$/} @sw;
  @list = keys %ip_addr
    if $REMOTE;

  for my $item (@list) {
    my $node;
    if ($REMOTE) {
      $node = $ip_addr{$item};
      my_print "dpctl tcp:$item $cmd ...\n";
      open IN, "dpctl tcp:$item $cmd|";
    } else {
      next unless $item =~ m|tmp/(s[0-9]+)$|;
      $node = $1;
      open IN, "sudo dpctl unix:$item $cmd|";
    }
    $db{$node} = {};
    while (<IN>) {
      my $line = $_;
      my $port = 0;
      while ($line =~ m/$var="(.*?)",/) {
	$db{$node}->{$port++} = $1;
	$line =~ s/$var//;
      }
    }
    close IN;
  }
  my $max = 0;
  for my $node (sort keys %db) {
    my_print "$node\t";
    my @ports = keys %{$db{$node}};
    my $len = scalar @ports;
    $max = $len if $len > $max;
  }
  my_print "\n";
  my $i;
  for ($i=0; $i<$max; $i++) {
    for my $node (sort keys %db) {
      my $val = ".";
      $val = $db{$node}->{$i}
	if defined $db{$node}->{$i};
      my_print "$val\t";
    }
    my_print "\n";
  }

  return \%db;
}

my $layout = 'normal';
sub change_layout {

  my $new_layout;
  if (-f '/tmp/topo_greedy') {
    $new_layout = 'greedy';
  } else {
    $new_layout = 'normal';
  }
  return
    if $new_layout eq $layout;
  $layout = $new_layout;

  my $pos_new = \%pos_nc;
  my $pos_old = \%pos_greedy;
  if ($layout eq 'greedy') {
    $pos_old = \%pos_nc;
    $pos_new = \%pos_greedy;
  }

  set_nodes($pos_new);
  foreach my $node (keys %$pos_new) {
    # should be relative to the ORIGINAL position.
    my $x = 55 * ($pos_new->{$node}[0] - $pos_nc{$node}[0]);
    my $y = 55 * ($pos_new->{$node}[1] - $pos_nc{$node}[1]);

    my $rect = $nodes{$node}[2];
    $rect->animate($x, $y, 1, 0, TRUE, 3000, 100, 'freeze');
  }
  foreach my $node_src (keys %ports) {
    foreach my $node_dst (@{$ports{$node_src}}) {
      remove_link($root, $node_src, $node_dst);
    }
  }

  $timer_link = Glib::Timeout->add(3 * 1000, \&add_links);
}

my $db;
sub update {
  my $db_old = $db;
  $db = get_stats();
  my $pkt_all = 0;
  my $num_ports = 0;
  foreach my $node (keys %$db) {
    foreach my $port (keys %{$db->{$node}}) {
      my $node_dst = $ports{$node}[$port];
      next
	unless defined $node_dst;
      my $link = get_link($node, $node_dst);
      my $diff = $link->{diff};

      $diff += $db->{$node}->{$port};
      $diff -= $db_old->{$node}->{$port}
	if defined $db_old->{$node}->{$port};

      $link->{diff} = $diff;

      $pkt_all += $diff;
      $num_ports ++;
    }
  }
  return TRUE
    unless $num_ports > 0;

  my $threshold = 0.66 * $pkt_all / $num_ports;
  foreach my $link (values %links) {
    if ($link->{diff} > $threshold) {
      $link->set('stroke-color' => 'red');
    } else {
      $link->set('stroke-color' => 'black');
    }
    $link->{diff} = 0;
  }

  change_layout();

  return TRUE;
}

my $window = Gtk2::Window->new('toplevel');
$window->signal_connect('delete_event' => sub { Gtk2->main_quit; });
$window->set_default_size(330, 180);
$window->set_title("Traffic monitor");

my $swin = Gtk2::ScrolledWindow->new;
$swin->set_shadow_type('in');
$window->add($swin);

my $canvas = Goo::Canvas->new();
$canvas->set_size_request(330, 180);
$canvas->set_bounds(0, 0, 330, 180);
$swin->add($canvas);
$root = $canvas->get_root_item();

set_nodes(\%pos_nc);
add_links();

foreach my $node (keys %nodes) {
  my $x = $nodes{$node}[0];
  my $y = $nodes{$node}[1];

  add_node($root, $x, $y, $node);
}

my $timer = Glib::Timeout->add(2 * 1000, \&update);

$window->show_all();
Gtk2->main;

sub on_rect_button_press {
    my_print "Rect item pressed!\n";
    return TRUE;
}
