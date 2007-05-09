/* $Id: test_router_flexml.cpp,v 1.20 2007-05-09 21:23:09 adam Exp $
   Copyright (c) 2005-2007, Index Data.

This file is part of Metaproxy.

Metaproxy is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Metaproxy is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Metaproxy; see the file LICENSE.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
 */

#include "config.hpp"
#include <iostream>
#include <stdexcept>

#include "filter.hpp"
#include "router_flexml.hpp"
#include "factory_static.hpp"

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;

namespace mp = metaproxy_1;

static int tfilter_ref = 0;
class TFilter: public mp::filter::Base {
public:
    void process(mp::Package & package) const {};
    TFilter() { tfilter_ref++; };
    ~TFilter() { tfilter_ref--; };
};

static mp::filter::Base* filter_creator()
{
    return new TFilter;
}

// Pass well-formed XML and valid configuration to it (implicit NS)
BOOST_AUTO_UNIT_TEST( test_router_flexml_1 )
{
    try
    {
        std::string xmlconf = "<?xml version=\"1.0\"?>\n"
            "<metaproxy xmlns=\"http://indexdata.com/metaproxy\""
            " version=\"1.0\">\n"
            "  <start route=\"start\"/>\n"
            "  <filters>\n"
            "    <filter id=\"front_default\" type=\"frontend_net\">\n"
            "      <port>@:210</port>\n"
            "    </filter>\n"
            "    <filter id=\"log_cout1\" type=\"log\">\n"
            "      <message>my msg</message>\n"
            "    </filter>\n"
            "    <filter id=\"tfilter_id\" type=\"tfilter\"/>\n"
            "    <filter id=\"log_cout2\" type=\"log\">\n"
            "      <message>other</message>\n"
            "    </filter>\n"
            "  </filters>\n"
            "  <routes>\n"  
            "    <route id=\"start\">\n"
            "      <filter refid=\"front_default\"/>\n"
            "      <filter refid=\"log_cout1\"/>\n"
            "      <filter type=\"tfilter\">\n"
            "      </filter>\n"
            "      <filter type=\"z3950_client\">\n"
            "      </filter>\n"
            "    </route>\n"
            "  </routes>\n"
            "</metaproxy>\n";

        mp::FactoryStatic factory;
        factory.add_creator("tfilter", filter_creator);
        mp::RouterFleXML rflexml(xmlconf, factory);
        BOOST_CHECK_EQUAL(tfilter_ref, 2);
    }
    catch ( std::runtime_error &e) {
        std::cout << "std::runtime error: " << e.what() << "\n";
        BOOST_CHECK (false);
    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
    BOOST_CHECK_EQUAL(tfilter_ref, 0);
}

// Pass non-wellformed XML
BOOST_AUTO_UNIT_TEST( test_router_flexml_2 )
{
    bool got_error_as_expected = false;
    try
    {
        std::string xmlconf_invalid = "<?xml version=\"1.0\"?>\n"
            "<mp:metaproxy xmlns:mp=\"http://indexdata.com/metaproxy\" version=\"1.0\">\n"
            "  <start route=\"start\"/>\n"
            "  <filters>\n"
            "    <filter id=\"front_default\" type=\"frontend_net\">\n"
            "      <port>@:210</port>\n";
        
        mp::FactoryFilter factory;
        mp::RouterFleXML rflexml(xmlconf_invalid, factory);
    }
    catch ( mp::XMLError &e) {
        std::cout << "XMLError: " << e.what() << "\n";
        got_error_as_expected = true;
    }
    catch ( std::runtime_error &e) {
        std::cout << "std::runtime error: " << e.what() << "\n";
    }
    catch ( ... ) {
        ;
    }
    BOOST_CHECK(got_error_as_expected);
}

// Pass well-formed XML with explicit NS
BOOST_AUTO_UNIT_TEST( test_router_flexml_3 )
{
    try
    {
        std::string xmlconf = "<?xml version=\"1.0\"?>\n"
            "<mp:metaproxy xmlns:mp=\"http://indexdata.com/metaproxy\""
            "  version=\"1.0\">\n"
            "  <mp:start route=\"start\"/>\n"
            "  <mp:filters>\n"
            "    <mp:filter id=\"front_default\" type=\"frontend_net\">\n"
            "      <port>@:210</port>\n"
            "    </mp:filter>\n"
            "    <mp:filter id=\"log_cout\" type=\"log\">\n"
            "      <message>my msg</message>\n"
            "    </mp:filter>\n"
            "  </mp:filters>\n"
            "  <mp:routes>\n"  
            "    <mp:route id=\"start\">\n"
            "      <mp:filter refid=\"front_default\"/>\n"
            "      <mp:filter refid=\"log_cout\"/>\n"
            "    </mp:route>\n"
            "  </mp:routes>\n"
            "</mp:metaproxy>\n";
       
        mp::FactoryStatic factory;
        mp::RouterFleXML rflexml(xmlconf, factory);
    }
    catch ( std::runtime_error &e) {
        std::cout << "std::runtime error: " << e.what() << "\n";
        BOOST_CHECK (false);
    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
}

// Pass well-formed XML but bad filter type
BOOST_AUTO_UNIT_TEST( test_router_flexml_4 )
{
    bool got_error_as_expected = false;
    try
    {
        std::string xmlconf = "<?xml version=\"1.0\"?>\n"
            "<metaproxy xmlns=\"http://indexdata.com/metaproxy\""
            " version=\"1.0\">\n"
            "  <start route=\"start\"/>\n" 
            "  <filters>\n"
            "    <filter id=\"front_default\" type=\"notknown\">\n"
            "      <port>@:210</port>\n"
            "    </filter>\n"
            "  </filters>\n"
            "  <routes>\n"  
            "    <route id=\"start\">\n"
            "      <filter refid=\"front_default\"/>\n"
            "    </route>\n"
            "  </routes>\n"
            "</metaproxy>\n";

        mp::FactoryStatic factory;
        factory.add_creator("tfilter", filter_creator);
        mp::RouterFleXML rflexml(xmlconf, factory);
    }
    catch ( mp::FactoryFilter::NotFound &e) {
        std::cout << "mp::FactoryFilter::NotFound: " << e.what() << "\n";
        got_error_as_expected = true;
    }
    catch ( std::runtime_error &e) {
        std::cout << "std::runtime error: " << e.what() << "\n";
    }
    BOOST_CHECK(got_error_as_expected);
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
