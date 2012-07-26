/* Copyright 2012 (C) Budapest University of Technology and Economics
 *
 * This file is NOT part of NOX.
 *
 * Butterfly_app is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NOX.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Author: Felicián Németh <nemethf@tmit.bme.hu>
 */

#include <boost/bind.hpp>
#include <fstream>
#include <iostream>
#include <utility>
#include <unordered_map>
#include "assert.hh"
#include "datapath-join.hh"
#include <openflow/bme-ext.h>
#include "../oflib/ofl-packets.h"
#include "../oflib-exp/ofl-exp-bme.h"
#include "packets.h"
#include "butterfly_app.hh"

namespace vigil
{
  static Vlog_module lg("butterfly_app");

  const uint32_t h1_ip_addr = 0x0a000001;
  const uint32_t h2_ip_addr = 0x0a000002;
  const uint32_t h3_ip_addr = 0x0a000003;
  const uint32_t h4_ip_addr = 0x0a000004;

  const int s5_port_to_h1  = 1;
  const int s5_port_to_s7  = 2;
  const int s5_port_to_s9  = 3;
  const int s6_port_to_h2  = 1;
  const int s6_port_to_s7  = 2;
  const int s6_port_to_s10 = 3;
  const int s7_port_to_s5  = 1;
  const int s7_port_to_s6  = 2;
  const int s7_port_to_s8  = 3;
  const int s8_port_to_s7  = 1;
  const int s8_port_to_s9  = 2;
  const int s8_port_to_s10 = 3;
  const int s9_port_to_h3  = 1;
  const int s9_port_to_s5  = 2;
  const int s9_port_to_s8  = 3;
  const int s10_port_to_h4 = 1;
  const int s10_port_to_s6 = 2;
  const int s10_port_to_s8 = 3;

  void
  butterfly_app::b_send(b_flow_mod* b)
  {
    struct ofp_header *oh = b->build();
    send_openflow_command(dp_id, oh, true);
    delete b;
  }

  Disposition
  butterfly_app::general_join_handler(const Event& e0)
  {
    const Datapath_join_event& e 
      = assert_cast <const Datapath_join_event&> (e0);
    b_flow_mod *b;

#if OFP_VERSION == 0x01
    this->dp_id = e.datapath_id;
#else
    this->dp_id = e.dpid;
#endif
    lg.dbg(" general_join_handler called ============== pathid: %s ",
           dp_id.string().c_str());

    switch (dp_id.as_host()) {
    case 5: { /* ================================================== */
      b = new b_flow_mod();
      b->match_src( h1_ip_addr );
      b->match_dst( h3_ip_addr );
      b->apply_actions()->push_mpls_header()
                        ->set_mpls_label( 1 )
                        ->output( s5_port_to_s7 )
                        ->output( s5_port_to_s9 );
      b_send(b);

      b = new b_flow_mod();
      b->write_actions()->output( s5_port_to_h1 );
      b_send(b);

      break;
    }
    case 6: { /* ================================================== */
      b = new b_flow_mod();
      b->match_src( h2_ip_addr );
      b->match_dst( h4_ip_addr );
      b->apply_actions()->push_mpls_header()
                        ->set_mpls_label( 2 )
                        ->output( s6_port_to_s7 )
                        ->output( s6_port_to_s10 );
      b_send(b);

      b = new b_flow_mod();
      b->write_actions()->output( s6_port_to_h2 );
      b_send(b);

      break;
    }
    case 7: { /* ================================================== */
      b = new b_flow_mod();
      b->match_mpls_label( 1 );
      b->write_actions()->output( s7_port_to_s8 );
      b_send(b);

      b = new b_flow_mod();
      b->match_mpls_label( 2 );
      b->write_actions()->output( s7_port_to_s8 );
      b_send(b);

      break;
    }
    case 8: { /* ================================================== */
      b_flow_mod *b = new b_flow_mod();
      b->match_mpls_label( 1 );
      b->write_actions()->output( s8_port_to_s10 );
      b_send(b);

      b = (new b_flow_mod());
      b->match_mpls_label( 2 );
      b->write_actions()->output( s8_port_to_s9 );
      b_send(b);

      break;
    }
    case 9: { /* ================================================== */
      b_flow_mod *b = new b_flow_mod();
      b->match_mpls_label( 1 );
      b->write_actions()->pop_mpls_header()
                        ->output( s9_port_to_h3 );
      b_send(b);


      b = new b_flow_mod();
      b->match_mpls_label( 2 );
      b->write_actions()->pop_mpls_header()
                        ->set_ipv4_destination( h3_ip_addr )
                        ->output( s9_port_to_h3 );
      b_send(b);

      b = new b_flow_mod();
      b->write_actions()->output( s9_port_to_s5 );
      b_send(b);

      break;
    }
    case 10: { /* ================================================== */
      b = new b_flow_mod();
      b->match_mpls_label( 2 );
      b->write_actions()->pop_mpls_header()
                        ->output( s10_port_to_h4 );
      b_send(b);

      b = new b_flow_mod();
      b->match_mpls_label( 1 );
      b->write_actions()->pop_mpls_header()
                        ->set_ipv4_destination( h4_ip_addr )
                        ->output( s10_port_to_h4 );
      b_send(b);

      b = new b_flow_mod();
      b->write_actions()->output( s10_port_to_s6 );
      b_send(b);

      break;
    }
    case 1: { /* ================================================== */
      /* test topo:                  */
      /*  s1 <-> h2-eth0 h3-eth0     */
      /*  s1: intfs=s1-eth1,s1-eth2  */
      //send_set_mpls_from_counter(dp_id, 2);
      //send_set_metadata(dp_id, OFPFMF_IN_PORT, 0, 2);
      //send_set_metadata(dp_id, OFPFMF_NW_PROTO, 4, 2);
      //send_set_metadata(dp_id, OFPFMF_MPLS_LABEL, 16, 2);
      //send_set_metadata(dp_id, OFPFMF_DL_SRC, 0, 2);
      //send_set_output(dp_id);

      break;
    }
    default:
      lg.warn("\n\n UNKNOWN PATHID, IGNORED -=-=-=-=-=-=-=-=-=-=-=-=-=\n\n");

      break;
    }

    return CONTINUE;
  }

  Disposition
  butterfly_app::network_coding_join_handler(const Event& e0)
  {
    const Datapath_join_event& e 
      = assert_cast <const Datapath_join_event&> (e0);
    b_flow_mod *b;

    this->dp_id = e.dpid;
    lg.dbg(" network_coding_join_handler called =========== pathid: %s ",
           dp_id.string().c_str());

    switch (dp_id.as_host()) {
    case 5: { /* ================================================== */
      b = new b_flow_mod();
      b->match_src( h1_ip_addr );
      b->match_dst( h3_ip_addr );
      b->apply_actions()->push_mpls_header()
                        ->set_mpls_label( 0 )
                        ->push_mpls_header()
                        ->set_mpls_label_from_counter()
                        ->push_mpls_header()
                        ->set_mpls_label( 1 )
                        ->output( s5_port_to_s7 )
                        ->output( s5_port_to_s9 );
      b_send(b);

      b = new b_flow_mod();
      b->write_actions()->output( s5_port_to_h1 );
      b_send(b);

      break;
    }
    case 6: { /* ================================================== */
      b = new b_flow_mod();
      b->match_src( h2_ip_addr );
      b->match_dst( h4_ip_addr );
      b->apply_actions()->push_mpls_header()
                        ->set_mpls_label_from_counter()
                        ->push_mpls_header()
                        ->set_mpls_label( 0 )
                        ->push_mpls_header()
                        ->set_mpls_label( 2 )
                        ->output( s6_port_to_s7 )
                        ->output( s6_port_to_s10 );
      b_send(b);

      b = new b_flow_mod();
      b->write_actions()->output( s6_port_to_h2 );
      b_send(b);

      break;
    }
    case 7: { /* ================================================== */
      b = new b_flow_mod();
      b->match_mpls_label( 1 );
      b->apply_actions()->set_mpls_label( 3 )
                        ->xor_encode( 3, 11 );
      b_send(b);

      b = new b_flow_mod();
      b->match_mpls_label( 2 );
      b->apply_actions()->set_mpls_label( 3 )
                        ->xor_encode( 3, 12 );
      b_send(b);

      b = new b_flow_mod();
      b->match_mpls_label( 3 );
      b->write_actions()->output( s7_port_to_s8 );
      b_send(b);
      
      b = new b_flow_mod();
      b->match_mpls_label( 11 );
      b->write_actions()->set_mpls_label( 1 )
                        ->output( s7_port_to_s8 );
      b_send(b);

      b = new b_flow_mod();
      b->match_mpls_label( 12 );
      b->write_actions()->set_mpls_label( 2 )
                        ->output( s7_port_to_s8 );
      b_send(b);

      /* for debugging purposes */
      b = new b_flow_mod();
      b->write_actions()->output( s7_port_to_s8 );
      b_send(b);

      break;
    }
    case 8: { /* ================================================== */
      b = new b_flow_mod();
      b->match_mpls_label( 1 );
      b->write_actions()->output( s8_port_to_s10 );
      b_send(b);

      b = new b_flow_mod();
      b->match_mpls_label( 2 );
      b->write_actions()->output( s8_port_to_s9 );
      b_send(b);

      b = new b_flow_mod();
      b->match_mpls_label( 3 );
      b->apply_actions()->output( s8_port_to_s9 )
                        ->output( s8_port_to_s10 );
      b_send(b);

      break;
    }
    case 9: { /* ================================================== */
      b = new b_flow_mod();
      b->match_mpls_label( 1 );
      b->apply_actions()->set_mpls_label( 3 )
                        ->xor_decode( 13, 23 )
                        ->pop_mpls_header( ETH_TYPE_MPLS )
                        ->pop_mpls_header( ETH_TYPE_MPLS )
                        ->pop_mpls_header( ETH_TYPE_IP )
                        ->output( s9_port_to_h3 );
      b_send(b);

      b = new b_flow_mod();
      b->match_mpls_label( 2 );
      b->apply_actions()->set_mpls_label( 3 )
                        ->xor_decode( 13, 23 )
                        ->pop_mpls_header( ETH_TYPE_MPLS )
                        ->pop_mpls_header( ETH_TYPE_MPLS )
                        ->pop_mpls_header( ETH_TYPE_IP )
                        ->set_ipv4_destination( h3_ip_addr )
                        ->output( s9_port_to_h3 );
      b_send(b);

      b = new b_flow_mod();
      b->match_mpls_label( 3 );
      b->apply_actions()->xor_decode( 13, 23 );
      b_send(b);

      b = new b_flow_mod();
      b->match_mpls_label( 13 );
      b->apply_actions()->pop_mpls_header( ETH_TYPE_MPLS )
                        ->pop_mpls_header( ETH_TYPE_MPLS )
                        ->pop_mpls_header( ETH_TYPE_IP )
                        ->set_ipv4_destination( h3_ip_addr )
                        ->output( s9_port_to_h3 );
      b_send(b);

      b = new b_flow_mod();
      b->match_mpls_label( 23 );
      b->apply_actions()->pop_mpls_header( ETH_TYPE_MPLS )
                        ->pop_mpls_header( ETH_TYPE_MPLS )
                        ->pop_mpls_header( ETH_TYPE_IP )
                        ->set_ipv4_destination( h3_ip_addr )
                        ->output( s9_port_to_h3 );
      b_send(b);

      b = new b_flow_mod();
      b->write_actions()->output( s9_port_to_s5 );
      b_send(b);

      break;
    }
    case 10: { /* ================================================== */
      b = new b_flow_mod();
      b->match_mpls_label( 1 );
      b->apply_actions()->set_mpls_label( 3 )
                        ->xor_decode( 13, 23 )
                        ->pop_mpls_header( ETH_TYPE_MPLS )
                        ->pop_mpls_header( ETH_TYPE_MPLS )
                        ->pop_mpls_header( ETH_TYPE_IP )
                        ->set_ipv4_destination( h4_ip_addr )
                        ->output( s10_port_to_h4 );
      b_send(b);

      b = new b_flow_mod();
      b->match_mpls_label( 2 );
      b->apply_actions()->set_mpls_label( 3 )
                        ->xor_decode( 13, 23 )
                        ->pop_mpls_header( ETH_TYPE_MPLS )
                        ->pop_mpls_header( ETH_TYPE_MPLS )
                        ->pop_mpls_header( ETH_TYPE_IP )
                        ->output( s10_port_to_h4 );
      b_send(b);

      b = new b_flow_mod();
      b->match_mpls_label( 3 );
      b->apply_actions()->xor_decode( 13, 23 );
      b_send(b);

      b = new b_flow_mod();
      b->match_mpls_label( 13 );
      b->apply_actions()->pop_mpls_header( ETH_TYPE_MPLS )
                        ->pop_mpls_header( ETH_TYPE_MPLS )
                        ->pop_mpls_header( ETH_TYPE_IP )
                        ->set_ipv4_destination( h4_ip_addr )
                        ->output( s10_port_to_h4 );
      b_send(b);

      b = new b_flow_mod();
      b->match_mpls_label( 23 );
      b->apply_actions()->pop_mpls_header( ETH_TYPE_MPLS )
                        ->pop_mpls_header( ETH_TYPE_MPLS )
                        ->pop_mpls_header( ETH_TYPE_IP )
                        ->set_ipv4_destination( h4_ip_addr )
                        ->output( s10_port_to_h4 );
      b_send(b);

      b = new b_flow_mod();
      b->write_actions()->output( s10_port_to_s6 );
      b_send(b);

      break;
    }
    default:
      lg.warn("\n\n UNKNOWN PATHID, IGNORED -=-=-=-=-=-=-=-=-=-=-=-=-=\n\n");

      break;
    }

    return CONTINUE;
  }

  Disposition
  butterfly_app::greedy_routing_join_handler(const Event& e0)
  {
    const Datapath_join_event& e 
      = assert_cast <const Datapath_join_event&> (e0);
    
    this->dp_id = e.dpid;
    lg.dbg(" greedy_routing_join_handler called =========== pathid: %s ",
           dp_id.string().c_str());

    if (links.find(dp_id.as_host()) == links.end()) {
      lg.warn("\n\n UNKNOWN PATHID, IGNORED -=-=-=-=-=-=-=-=-=-=-=-=-=\n\n");
      return CONTINUE;
    }
    uint32_t from_node = dp_id.as_host();
    port_list_t *l = &links[ from_node ];

    b_flow_mod *b = new b_flow_mod();
    b->write_metadata( 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL );
    
    for(port_list_t::iterator i = l->begin(); i != l->end(); i++) {
      port_t p = *i;
      uint32_t to_node = std::get<0>(p);
      uint32_t port_no = std::get<1>(p);

      coord_t c = greedy_coords[ to_node ];
      uint32_t x = std::get<0>(c);
      uint32_t y = std::get<1>(c);
      //lg.dbg(" node,x,y,port_no: %d,%d,%d,%d ", from_node, x, y, port_no);

      b->apply_actions()
            ->update_distance_in_metadata( x, y, port_no );
    }
    b->write_metadata( 0x0000000000000000ULL, 0xFFFFFFFFFFFF0000ULL );
    b->apply_actions()->output_by_metadata();
    b_send(b);
    
    return CONTINUE;
  }

  void
  butterfly_app::bloom_fill_table(int table_id, int port_no,
                                  uint64_t bloom_addr, uint64_t eth_dst_addr,
                                  uint32_t ip_dst_addr)
  {
    b_flow_mod *b;

    b = new b_flow_mod();
    b->table( table_id );
    b->match_eth_dst( bloom_addr, ~bloom_addr );
    if (eth_dst_addr)
      b->apply_actions()->set_eth_dst( eth_dst_addr )
                        ->set_ipv4_destination( ip_dst_addr );
    b->apply_actions()->output( port_no );
    b->instructions()->goto_table( table_id + 1 );
    b_send(b);

    b = new b_flow_mod();
    b->table( table_id );
    b->instructions()->goto_table( table_id + 1 );
    b_send(b);
  }

  Disposition
  butterfly_app::bloom_filter_join_handler(const Event& e0)
  {
    const Datapath_join_event& e 
      = assert_cast <const Datapath_join_event&> (e0);

    this->dp_id = e.dpid;
    lg.dbg(" bloom_filter_join_handler called =========== pathid: %s ",
           dp_id.string().c_str());

    if (links.find(dp_id.as_host()) == links.end()) {
      lg.warn("\n\n UNKNOWN PATHID, IGNORED -=-=-=-=-=-=-=-=-=-=-=-=-=\n\n");
      return CONTINUE;
    }

    b_flow_mod *b = new b_flow_mod();
    b->table( 0 );
    b->apply_actions()->decrement_ipv4_ttl();
    b->instructions()->goto_table( 1 );
    b_send(b);

    uint32_t from_node = dp_id.as_host();
    port_list_t *l = &links[ from_node ];
    int num_links = 0;
    for(port_list_t::iterator i = l->begin(); i != l->end(); i++) {
      port_t p = *i;
      uint32_t to_node = std::get<0>(p);
      uint32_t port_no = std::get<1>(p);
      uint64_t addr    = std::get<2>(p);

      lg.dbg("flowmod: %d, %d, 0x%"PRIx64, to_node, port_no, addr);

      uint64_t host_eth = 0;
      uint32_t host_ip  = 0;
      port_list_t *l_to = &links[ to_node ];
      if (l_to->size() == 1) {
        host_eth = std::get<2>( l_to->front() );
        host_ip  = 0x0a000000 + to_node;  // assuming: 10.0.0.id
      }

      bloom_fill_table( ++num_links, port_no, addr, host_eth, host_ip );
    }
    
    return CONTINUE;
  }

  Disposition
  butterfly_app::datapath_join_handler(const Event& e0)
  {
    switch (type) {
    case MPLS_MULTICAST: {return general_join_handler(e0);}
    case NETWORK_CODING: {return network_coding_join_handler(e0);}
    case GREEDY_ROUTING: {return greedy_routing_join_handler(e0);}
    case BLOOM_FILTER:   {return bloom_filter_join_handler(e0);}
    default: {
      lg.warn("unknown app_type (%d)", type);
      return CONTINUE;
    }
    }
  }

  void butterfly_app::configure(const Configuration* c) 
  {
    lg.dbg(" Configure called ");
    const Component_argument_list args = c->get_arguments();
    
    Component_argument_list::const_iterator arg;
    for (arg = args.begin(); arg != args.end(); ++arg) {
      lg.dbg(" arg:%s ", arg->c_str());
      if (strcmp(arg->c_str(), "nc") == 0) {
	type = NETWORK_CODING;
	lg.dbg(" === NETWORK CODING ==== ");
        continue;
      }
      if (strcmp(arg->c_str(), "greedy") == 0) {
        type = GREEDY_ROUTING;
	lg.dbg(" === GREEDY ROUTING ==== ");
        continue;
      }
      if (strcmp(arg->c_str(), "bloom") == 0) {
        type = BLOOM_FILTER;
	lg.dbg(" === BLOOM FILTERS ==== ");
        continue;
      }
      if (strncmp(arg->c_str(), "links=", 6) == 0) {
        uint32_t to_id, port_no;
        uint64_t addr;
        char c;
        ifstream stream(arg->c_str() + 6);
         if (!stream) {
           lg.err(" error opening: %s ", arg->c_str());
           continue;
         }
         while (stream) {
           // This is very fragile.  The following format is assumed:
           // from_id, to_id, port_no, addr_in_hex\n
           // e.g.: 1, 5, 1, 820000700000
           uint32_t from_id = 0;
           stream >> dec >> from_id >> c >> to_id >> c 
                  >> port_no >> c >> hex >> addr;
           if (from_id == 0)
             continue;
           //lg.dbg("0x%.12llx, %lu", addr, from_id);

           port_list_t *l;
           if (links.find(from_id) != links.end()) {
             l = &links[from_id];
           } else {
             l = new port_list_t;
           }
           l->push_back( make_tuple(to_id, port_no, addr) );
           links[ from_id ] = *l;
         }
      }
      if (strncmp(arg->c_str(), "coords=", 7) == 0) {
        uint32_t x, y;
        char c;
        ifstream stream(arg->c_str() + 7);
         if (!stream) {
           lg.err(" error opening: %s ", arg->c_str());
           continue;
         }
         while (stream) {
           // This is very fragile.  The following format is assumed:
           // node_id, x_coordinate, y_coordinate\n
           // e.g.: 1, 2, 3
           uint32_t node_id = 0;
           stream >> dec >> node_id >> c >> x >> c >> y;
           if (node_id == 0)
             continue;
           //lg.dbg(" %d %d %d ", node_id, x, y);

           greedy_coords[ node_id ] = make_tuple(x, y);
         }
      }
    }
  }
  
  void butterfly_app::install()
  {
    lg.dbg(" Install called ");

    register_handler<Datapath_join_event>
      (boost::bind(&butterfly_app::datapath_join_handler, this, _1));
  }

  void butterfly_app::getInstance(const Context* c,
				  butterfly_app*& component)
  {
    component = dynamic_cast<butterfly_app*>
      (c->get_by_interface(container::Interface_description
			      (typeid(butterfly_app).name())));
  }

  REGISTER_COMPONENT(Simple_component_factory<butterfly_app>,
		     butterfly_app);
} // vigil namespace
