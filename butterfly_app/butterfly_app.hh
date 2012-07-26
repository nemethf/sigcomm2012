/* Copyright 2008 (C) Nicira, Inc.
 * Copyright 2009 (C) Stanford University.
 * Copyright 2012 (C) Budapest University of Technology and Economics.
 *
 * This file is part of NOX.
 *
 * NOX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * NOX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NOX.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef butterfly_app_HH
#define butterfly_app_HH

#include "component.hh"
#include "config.h"
#include "ofp_builder.hh"

#ifdef LOG4CXX_ENABLED
#include <boost/format.hpp>
#include "log4cxx/logger.h"
#else
#include "vlog.hh"
#endif

namespace vigil
{
  using namespace std;
  using namespace vigil::container;

  enum app_type {
    MPLS_MULTICAST,
    NETWORK_CODING,
    GREEDY_ROUTING,
    BLOOM_FILTER,
  };

  typedef std::tuple<uint32_t, uint32_t, uint64_t> port_t;
  typedef std::list<port_t> port_list_t;
  typedef std::unordered_map<uint32_t, port_list_t> links_t;
  typedef std::tuple<uint32_t, uint32_t> coord_t;
  typedef std::unordered_map<uint32_t, coord_t> coord_map_t;

  /** \brief butterfly_app
   * \ingroup noxcomponents
   * 
   * @author nemethf@tmit.bme.hu
   * @date
   */
  class butterfly_app
    : public Component 
  {
  public:
    /** \brief Constructor of butterfly_app.
     *
     * @param c context
     * @param node XML configuration (JSON object)
     */
    butterfly_app(const Context* c, const json_object* node)
      : Component(c), type(MPLS_MULTICAST)
    {}

    Disposition
    datapath_join_handler(const Event& e);
    
    /** \brief Configure butterfly_app.
     * 
     * Parse the configuration, register event handlers, and
     * resolve any dependencies.
     *
     * @param c configuration
     */
    void configure(const Configuration* c);

    /** \brief Start butterfly_app.
     * 
     * Start the component. For example, if any threads require
     * starting, do it now.
     */
    void install();

    /** \brief Get instance of butterfly_app.
     * @param c context
     * @param component reference to component
     */
    static void getInstance(const container::Context* c, 
			    butterfly_app*& component);

  private:
    enum app_type type;
    datapathid dp_id;
    links_t links;
    coord_map_t greedy_coords;

    Disposition general_join_handler(const Event& e);
    Disposition network_coding_join_handler(const Event& e);
    Disposition greedy_routing_join_handler(const Event& e);
    Disposition bloom_filter_join_handler(const Event& e);

    void bloom_fill_table(int table_id, int port_no, uint64_t bloom_addr,
                          uint64_t eth_addr = 0, uint32_t ip_addr = 0 );

    uint32_t get_new_xid();
    uint64_t hton_48(uint64_t addr);

    void b_send(b_flow_mod* b);
  };
}

#endif
