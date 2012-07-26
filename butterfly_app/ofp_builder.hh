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

#ifndef opf_builder_HH
#define opf_builder_HH

#include <ostream>
#include "netinet++/datapathid.hh"
#include "../oflib/ofl-messages.h"
#include "packets.h"

#ifdef LOG4CXX_ENABLED
#include <boost/format.hpp>
#include "log4cxx/logger.h"
#else
#include "vlog.hh"
#endif

namespace vigil
{
  class b_instructions;
  class b_apply_actions;
  class b_actions;

  class b_flow_mod
  {
  public:
    b_flow_mod();
    ~b_flow_mod();

    b_flow_mod* table(uint8_t table_id);
    b_flow_mod* priority(uint16_t priority);
    b_flow_mod* match_mpls_label(uint32_t label);
    b_flow_mod* match_src(uint32_t addr);
    b_flow_mod* match_dst(uint32_t addr);
    b_flow_mod* match_eth_dst(uint64_t addr, uint64_t mask);
    b_flow_mod* match_metadata(uint64_t metadata, uint64_t mask);

    b_instructions* instructions();
    b_instructions* write_metadata(uint64_t metadata, uint64_t mask);
    b_actions* apply_actions();
    b_actions* write_actions();
    struct ofp_header* build();

  private:
    static uint32_t xid;
    struct ofl_msg_flow_mod ofl;
    struct ofl_match_standard match;
    b_instructions *instr;
    uint8_t *buffer;

    uint32_t get_new_xid() { return ++xid; }
  };

  class b_instructions
  {
  public:
    b_instructions(b_flow_mod *parent);
    ~b_instructions();

    b_instructions* New(int type = 0);
    b_instructions* goto_table(uint8_t table_id);
    b_instructions* write_metadata(uint64_t metadata, uint64_t mask);
    b_actions*      write_actions();
    b_actions*      apply_actions();
    b_instructions* clear_actions();
    b_flow_mod*     end();

    int get_num();
    struct ofl_instruction_header **build();

  private:
    ofl_instruction_header *ofl;
    b_instructions *next, *last;
    b_actions *actions;
    b_flow_mod *parent;
  };

  class b_actions
  {
  public:
    b_actions(b_instructions* parent);
    ~b_actions();

    b_actions* New();
    b_actions* output(int port_no);
    b_actions* set_mpls_label(uint32_t label);
    b_actions* decrement_mpls_ttl();
    b_actions* decrement_ipv4_ttl();
    b_actions* set_field_from_metadata(uint32_t field, uint8_t offset);
    b_actions* set_metadata_from_packet(uint32_t field, uint8_t offset);
    b_actions* set_metadata_from_counter(uint32_t max_num);
    b_actions* set_mpls_label_from_counter();
    b_actions* push_mpls_header();
    b_actions* pop_mpls_header(uint16_t ethertype = ETH_TYPE_IP);
    b_actions* set_eth_dst(uint64_t addr);
    b_actions* set_ipv4_destination(uint32_t dst);
    b_actions* output_by_metadata();
    b_actions* xor_encode(uint32_t label_a, uint32_t label_b);
    b_actions* xor_decode(uint32_t label_a, uint32_t label_b);
    b_actions* update_distance_in_metadata(uint32_t x, uint32_t y,
					   uint32_t port);
    b_actions* serialize(uint32_t mpls_label, uint32_t timeout);

    b_instructions* end();

    int get_num();
    struct ofl_action_header **build();
  private:
    ofl_action_header *ofl;
    b_actions *next, *last;
    b_instructions *parent;
  };
} // vigil namespace

#endif
