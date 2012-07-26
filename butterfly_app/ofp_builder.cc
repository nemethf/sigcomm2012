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

#include "ofp_builder.hh"
#include "../oflib/ofl.h"
#include "../oflib/ofl-packets.h"
#include <openflow/bme-ext.h>
#include "../oflib-exp/ofl-exp.h"
#include "../oflib-exp/ofl-exp-bme.h"
#include "packets.h"

namespace vigil
{
  static Vlog_module lg("butterfly_app");

  static struct ofl_exp_act  b_exp_act_callbacks;
  static struct ofl_exp_inst b_exp_inst_callbacks;
  static struct ofl_exp_msg  b_exp_msg_callbacks;
  static struct ofl_exp      b_ofl_exp;

  static struct ofl_exp*
  get_ofl_exp()
  {
    static int init = 0;

    if (init)
      return &b_ofl_exp;

    b_exp_act_callbacks.pack      = ofl_exp_act_pack;
    b_exp_act_callbacks.unpack    = ofl_exp_act_unpack;
    b_exp_act_callbacks.free      = ofl_exp_act_free;
    b_exp_act_callbacks.ofp_len   = ofl_exp_act_ofp_len;
    b_exp_act_callbacks.to_string = ofl_exp_act_to_string;

    b_exp_inst_callbacks.pack      = ofl_exp_inst_pack;
    b_exp_inst_callbacks.unpack    = ofl_exp_inst_unpack;
    b_exp_inst_callbacks.free      = ofl_exp_inst_free;
    b_exp_inst_callbacks.ofp_len   = ofl_exp_inst_ofp_len;
    b_exp_inst_callbacks.to_string = ofl_exp_inst_to_string;

    b_exp_msg_callbacks.pack      = ofl_exp_msg_pack;
    b_exp_msg_callbacks.unpack    = ofl_exp_msg_unpack;
    b_exp_msg_callbacks.free      = ofl_exp_msg_free;
    b_exp_msg_callbacks.to_string = ofl_exp_msg_to_string;

    b_ofl_exp.act   = &b_exp_act_callbacks;
    b_ofl_exp.inst  = &b_exp_inst_callbacks;
    b_ofl_exp.match = NULL;
    b_ofl_exp.stats = NULL;
    b_ofl_exp.msg   = &b_exp_msg_callbacks;
    
    init++;

    return &b_ofl_exp;
  }

  uint32_t b_flow_mod::xid = 0;

  static inline uint64_t
  hton_48(uint64_t addr)
  {
    uint64_t n = htonll(addr << 8) >> 8;
    return n;
  }

  b_flow_mod::b_flow_mod()
    : instr(0), buffer(0)
  {
    memset(&ofl, 0x00, sizeof ofl);
    ofl.header.type = OFPT_FLOW_MOD;
    ofl.command = OFPFC_ADD;
    ofl.idle_timeout = OFP_FLOW_PERMANENT;
    ofl.hard_timeout = OFP_FLOW_PERMANENT;
    ofl.priority = OFP_DEFAULT_PRIORITY;
    ofl.buffer_id = -1;
    ofl.out_port = OFPP_ANY;
    ofl.out_group = OFPG_ANY;
    ofl.match = (ofl_match_header*)&match;

    memset(&match, 0x00, sizeof match);
    match.header.type = OFPMT_STANDARD;
    match.wildcards = OFPFW_ALL;
    memset(&match.dl_src_mask, 0xFF, ETH_ADDR_LEN);
    memset(&match.dl_dst_mask, 0xFF, ETH_ADDR_LEN);
    match.nw_src_mask = 0xFFFFFFFF; /* IP source address mask. */
    match.nw_dst_mask = 0xFFFFFFFF; /* IP destination address mask. */
  }

  b_flow_mod*
  b_flow_mod::table(uint8_t table_id)
  {
    ofl.table_id = table_id;

    return this;
  }

  b_flow_mod*
  b_flow_mod::priority(uint16_t priority)
  {
    ofl.priority = priority;

    return this;
  }

  b_flow_mod*
  b_flow_mod::match_mpls_label(uint32_t mpls_label)
  {
    match.wildcards &= ~OFPFW_DL_TYPE;
    match.wildcards &= ~OFPFW_MPLS_LABEL;
    match.dl_type = ETH_TYPE_MPLS;
    match.mpls_label = mpls_label;

    return this;
  }

  b_flow_mod*
  b_flow_mod::match_src(uint32_t addr)
  {
    match.wildcards &= ~OFPFW_DL_TYPE;
    match.wildcards &= ~OFPFW_NW_PROTO;
    match.dl_type = ETH_TYPE_IP;
    match.nw_proto = IP_TYPE_UDP;
    match.nw_src = htonl(addr); // XXX
    match.nw_src_mask = 0x00000000;

    return this;
  }

  b_flow_mod*
  b_flow_mod::match_dst(uint32_t addr)
  {
    match.wildcards &= ~OFPFW_DL_TYPE;
    match.wildcards &= ~OFPFW_NW_PROTO;
    match.dl_type = ETH_TYPE_IP;
    match.nw_proto = IP_TYPE_UDP;
    match.nw_dst = htonl(addr); // XXX
    match.nw_dst_mask = 0x00000000;

    return this;
  }

  b_flow_mod*
  b_flow_mod::match_eth_dst(uint64_t addr, uint64_t mask)
  {
    addr = hton_48(addr);
    mask = hton_48(mask);

    memcpy(&match.dl_dst,      &addr, 6);
    memcpy(&match.dl_dst_mask, &mask, 6);

    return this;
  }

  b_flow_mod*
  b_flow_mod::match_metadata(uint64_t metadata, uint64_t mask)
  {
    match.metadata = metadata;
    match.metadata_mask = mask;

    return this;
  }

  b_instructions* 
  b_flow_mod::instructions()
  {
    if (instr == NULL) {
      instr = new b_instructions(this);
    }
    return instr;
  }

  b_instructions*
  b_flow_mod::write_metadata(uint64_t metadata, uint64_t mask)
  {
    return this->instructions()->write_metadata(metadata, mask);
  }

  b_actions*
  b_flow_mod::apply_actions()
  {
    return this->instructions()->apply_actions();
  }

  b_actions*
  b_flow_mod::write_actions()
  {
    return this->instructions()->write_actions();
  }

  struct ofp_header*
  b_flow_mod::build()
  {
    if (instr) {
      ofl.instructions_num = instr->get_num();
      ofl.instructions = instr->build();
    }

    size_t buf_size;
    int error = ofl_msg_pack((ofl_msg_header*)&ofl,
			     get_new_xid(), &buffer, &buf_size, get_ofl_exp());
    if (error) {
      lg.err("Error packing request.");
      exit(0);
    }

    return (struct ofp_header*)buffer;
  }

  b_flow_mod::~b_flow_mod()
  {
    delete instr;
    ofl_msg_free((struct ofl_msg_header *)buffer, get_ofl_exp());
  }

  // ----------------------------------------------------------------------

  b_instructions::b_instructions(b_flow_mod *parent)
    : ofl(0), next(0), last(this), actions(0), parent(parent)
  {
  }

  b_instructions*
  b_instructions::New(int type) {
    if (ofl == NULL)
      return this;

    b_instructions *old_last = this->last;
    if (type == OFPIT_APPLY_ACTIONS || type == OFPIT_WRITE_ACTIONS) {
      if (old_last->ofl->type == type)
	return old_last;
    }

    this->last =  new b_instructions(parent);
    old_last->next = this->last;

    return this->last;
  }

  b_instructions*
  b_instructions::goto_table(uint8_t table_id)
  {
    typedef struct ofl_instruction_goto_table ofl_t;
    last = this->New();
    last->ofl = (struct ofl_instruction_header*) new ofl_t;
    ofl_t *ofl = (ofl_t *)last->ofl;

    ofl->header.type = OFPIT_GOTO_TABLE;
    ofl->table_id = table_id;

    return this;
  }

  b_instructions*
  b_instructions::write_metadata(uint64_t metadata, uint64_t mask) {
    typedef struct ofl_instruction_write_metadata ofl_t;
    last = this->New();
    last->ofl = (struct ofl_instruction_header*) new ofl_t;
    ofl_t *ofl = (ofl_t*)last->ofl;

    ofl->header.type   = OFPIT_WRITE_METADATA;
    ofl->metadata      = metadata;
    ofl->metadata_mask = mask;

    return this;
  }

  b_actions*
  b_instructions::apply_actions()
  {
    typedef struct ofl_instruction_actions ofl_t;
    last = this->New(OFPIT_APPLY_ACTIONS);
    if (last->ofl)
      return last->actions;

    last->ofl = (struct ofl_instruction_header*) new ofl_t;
    ofl_t *ofl = (ofl_t *)last->ofl;

    ofl->header.type = OFPIT_APPLY_ACTIONS;
    ofl->actions_num = 0;
    ofl->actions = NULL;

    last->actions = new b_actions(this);

    return last->actions;
  }

  b_actions*
  b_instructions::write_actions()
  {
    typedef struct ofl_instruction_actions ofl_t;
    last = this->New(OFPIT_WRITE_ACTIONS);
    if (last->ofl)
      return last->actions;

    last->ofl = (struct ofl_instruction_header*) new ofl_t;
    ofl_t *ofl = (ofl_t *)last->ofl;

    ofl->header.type = OFPIT_WRITE_ACTIONS;
    ofl->actions_num = 0;
    ofl->actions = NULL;

    last->actions = new b_actions(this);

    return last->actions;
  }

  b_flow_mod*
  b_instructions::end()
  {
    return parent;
  }

  int
  b_instructions::get_num()
  {
    int i = 0;
    for (b_instructions *b = this; b; b = b->next) {
      i ++;
    }
    return i;
  }

  struct ofl_instruction_header **
  b_instructions::build()
  {
    for (b_instructions *b = this; b; b = b->next) {
      if (b->actions) {
	typedef struct ofl_instruction_actions ofl_t;
	ofl_t *ofl = (ofl_t *)b->ofl;
	ofl->actions_num = b->actions->get_num();
	ofl->actions     = b->actions->build();
      }
    }

    int i = 0;
    int num = get_num();
    struct ofl_instruction_header ** list;
    list = new struct ofl_instruction_header*[num];
    for (b_instructions *b = this; b; b = b->next, i++) {
      list[i] = b->ofl;
    }

    return list;
  }

  b_instructions::~b_instructions()
  {
    delete next;
    delete actions;
    delete ofl;
  }

  // ----------------------------------------------------------------------

  b_actions::b_actions(b_instructions *parent)
    : ofl(0), next(0), last(this), parent(parent)
  {
  }

  b_actions*
  b_actions::New() {
    if (ofl == NULL)
      return this;

    b_actions *old_last = this->last;
    this->last =  new b_actions(parent);
    old_last->next = this->last;

    return this->last;
  }

  b_actions*
  b_actions::output(int port_no) {
    typedef struct ofl_action_output ofl_t;
    last = this->New();
    last->ofl = (struct ofl_action_header*) new ofl_t;
    ofl_t *ofl = (ofl_t *)last->ofl;

    ofl->header.type = OFPAT_OUTPUT;
    ofl->port = port_no;
    ofl->max_len = 0;

    return this;
  }

  b_actions*
  b_actions::set_mpls_label(uint32_t label) {
    typedef struct ofl_action_mpls_label ofl_t;
    last = this->New();
    last->ofl = (struct ofl_action_header*) new ofl_t;
    ofl_t *ofl = (ofl_t*)last->ofl;

    ofl->header.type = OFPAT_SET_MPLS_LABEL;
    ofl->mpls_label = label;

    return this;
  }

  b_actions*
  b_actions::decrement_mpls_ttl() {
    typedef struct ofl_action_header ofl_t;
    last = this->New();
    last->ofl = (struct ofl_action_header*) new ofl_t;
    ofl_t *ofl = (ofl_t*)last->ofl;

    ofl->type = OFPAT_DEC_MPLS_TTL;

    return this;
  }

  b_actions*
  b_actions::decrement_ipv4_ttl() {
    typedef struct ofl_action_header ofl_t;
    last = this->New();
    last->ofl = (struct ofl_action_header*) new ofl_t;
    ofl_t *ofl = (ofl_t*)last->ofl;

    ofl->type = OFPAT_DEC_NW_TTL;

    return this;
  }

  b_actions*
  b_actions::set_field_from_metadata(uint32_t field, uint8_t offset) {
    typedef struct ofl_bme_set_metadata ofl_t;
    last = this->New();
    last->ofl = (struct ofl_action_header*) new ofl_t;
    ofl_t *ofl = (ofl_t*)last->ofl;

    ofl->header.type = OFPAT_EXPERIMENTER;
    ofl->experimenter_id = BME_EXPERIMENTER_ID;
    ofl->type = BME_SET_FIELD_FROM_METADATA;
    ofl->field = field;
    ofl->offset = offset;

    return this;
  }

  b_actions*
  b_actions::set_metadata_from_packet(uint32_t field, uint8_t offset) {
    typedef struct ofl_bme_set_metadata ofl_t;
    last = this->New();
    last->ofl = (struct ofl_action_header*) new ofl_t;
    ofl_t *ofl = (ofl_t*)last->ofl;

    ofl->header.type = OFPAT_EXPERIMENTER;
    ofl->experimenter_id = BME_EXPERIMENTER_ID;
    ofl->type = BME_SET_METADATA_FROM_PACKET;
    ofl->field = field;
    ofl->offset = offset;

    return this;
  }

  b_actions*
  b_actions::set_metadata_from_counter(uint32_t max_num) {
    typedef struct ofl_bme_set_metadata_from_counter ofl_t;
    last = this->New();
    last->ofl = (struct ofl_action_header*) new ofl_t;
    ofl_t *ofl = (ofl_t*)last->ofl;

    ofl->header.type = OFPAT_EXPERIMENTER;
    ofl->experimenter_id = BME_EXPERIMENTER_ID;
    ofl->type = BME_SET_METADATA_FROM_COUNTER;
    ofl->max_num = max_num;

    return this;
  }

  b_actions*
  b_actions::set_mpls_label_from_counter() {
    typedef struct ofl_bme_set_mpls_label ofl_t;
    last = this->New();
    last->ofl = (struct ofl_action_header*) new ofl_t;
    ofl_t *ofl = (ofl_t*)last->ofl;

    ofl->header.type = OFPAT_EXPERIMENTER;
    ofl->experimenter_id = BME_EXPERIMENTER_ID;
    ofl->type = BME_SET_MPLS_LABEL_FROM_COUNTER;

    return this;
  }

  b_actions*
  b_actions::push_mpls_header() {
    typedef struct ofl_action_push ofl_t;
    last = this->New();
    last->ofl = (struct ofl_action_header*) new ofl_t;
    ofl_t *ofl = (ofl_t*)last->ofl;

    ofl->header.type = OFPAT_PUSH_MPLS;
    ofl->ethertype = ETH_TYPE_MPLS;  /* or ETH_TYPE_MPLS_MCAST? */

    return this;
  }

  b_actions*
  b_actions::pop_mpls_header(uint16_t ethertype) 
  {
    typedef struct ofl_action_pop_mpls ofl_t;
    last = this->New();
    last->ofl = (struct ofl_action_header*) new ofl_t;
    ofl_t *ofl = (ofl_t*)last->ofl;

    ofl->header.type = OFPAT_POP_MPLS;
    ofl->ethertype = ethertype;

    return this;
  }

  b_actions*
  b_actions::set_eth_dst(uint64_t addr)
  {
    typedef struct ofl_action_dl_addr ofl_t;
    last = this->New();
    last->ofl = (struct ofl_action_header*) new ofl_t;
    ofl_t *ofl = (ofl_t*)last->ofl;

    addr = hton_48(addr);
    ofl->header.type = OFPAT_SET_DL_DST;
    memcpy(&ofl->dl_addr, &addr, 6);

    return this;
  }

  b_actions* 
  b_actions::set_ipv4_destination(uint32_t addr)
  {
    typedef struct ofl_action_nw_addr ofl_t;
    last = this->New();
    last->ofl = (struct ofl_action_header*) new ofl_t;
    ofl_t *ofl = (ofl_t*)last->ofl;

    ofl->header.type = OFPAT_SET_NW_DST;
    ofl->nw_addr = htonl(addr); // XXX

    return this;
  }

  b_actions*
  b_actions::output_by_metadata()
  {
    typedef struct ofl_bme_action_header ofl_t;
    last = this->New();
    last->ofl = (struct ofl_action_header*) new ofl_t;
    ofl_t *ofl = (ofl_t*)last->ofl;

    ofl->header.type = OFPAT_EXPERIMENTER;
    ofl->experimenter_id = BME_EXPERIMENTER_ID;
    ofl->type = BME_OUTPUT_BY_METADATA;

    return this;
  }

  b_actions*
  b_actions::xor_encode(uint32_t label_a, uint32_t label_b) {
    typedef struct ofl_bme_xor_packet ofl_t;
    last = this->New();
    last->ofl = (struct ofl_action_header*) new ofl_t;
    ofl_t *ofl = (ofl_t*)last->ofl;

    ofl->header.type = OFPAT_EXPERIMENTER;
    ofl->experimenter_id = BME_EXPERIMENTER_ID;
    ofl->type = BME_XOR_ENCODE;
    ofl->label_a = label_a;
    ofl->label_b = label_b;

    return this;
  }

  b_actions*
  b_actions::xor_decode(uint32_t label_a, uint32_t label_b) {
    typedef struct ofl_bme_xor_packet ofl_t;
    last = this->New();
    last->ofl = (struct ofl_action_header*) new ofl_t;
    ofl_t *ofl = (ofl_t*)last->ofl;

    ofl->header.type = OFPAT_EXPERIMENTER;
    ofl->experimenter_id = BME_EXPERIMENTER_ID;
    ofl->type = BME_XOR_DECODE;
    ofl->label_a = label_a;
    ofl->label_b = label_b;

    return this;
  }

  b_actions*
  b_actions::update_distance_in_metadata(uint32_t x, uint32_t y,
					 uint32_t port)
  {
    typedef struct ofl_bme_update_distance ofl_t;
    last = this->New();
    last->ofl = (struct ofl_action_header*) new ofl_t;
    ofl_t *ofl = (ofl_t*)last->ofl;

    ofl->header.type = OFPAT_EXPERIMENTER;
    ofl->experimenter_id = BME_EXPERIMENTER_ID;
    ofl->type = BME_UPDATE_DISTANCE_IN_METADATA;
    ofl->port = port;
    memcpy(&ofl->hw_addr, &x, 3);
    memcpy((char*)(&ofl->hw_addr) + 3, &y, 3);

    return this;
  }

  b_actions*
  b_actions::serialize(uint32_t mpls_label, uint32_t timeout)
  {
    typedef struct ofl_bme_serialize ofl_t;
    last = this->New();
    last->ofl = (struct ofl_action_header*) new ofl_t;
    ofl_t *ofl = (ofl_t*)last->ofl;

    ofl->header.type = OFPAT_EXPERIMENTER;
    ofl->experimenter_id = BME_EXPERIMENTER_ID;
    ofl->type = BME_SERIALIZE;
    ofl->mpls_label = mpls_label;
    ofl->timeout = timeout;

    return this;
  }

  b_instructions*
  b_actions::end()
  {
    return parent;
  }

  int
  b_actions::get_num()
  {
    int i = 0;
    for (b_actions *b = this; b; b = b->next) {
      i ++;
    }
    return i;
  }

  struct ofl_action_header **
  b_actions::build()
  {
    int i = 0;
    int num = get_num();
    struct ofl_action_header ** list;
    list = new struct ofl_action_header*[num];
    for (b_actions *b = this; b; b = b->next, i++) {
      list[i] = b->ofl;
    }

    return list;
  }

  b_actions::~b_actions()
  {
    delete next;
    delete ofl;
  }
}
 // vigil namespace
