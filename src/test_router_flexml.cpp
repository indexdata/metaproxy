/* $Id: test_router_flexml.cpp,v 1.15 2006-01-11 14:58:28 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
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

static int tfilter_ref = 0;
class TFilter: public yp2::filter::Base {
public:
    void process(yp2::Package & package) const {};
    TFilter() { tfilter_ref++; };
    ~TFilter() { tfilter_ref--; };
};

static yp2::filter::Base* filter_creator()
{
    return new TFilter;
}

// Pass well-formed XML and valid configuration to it (implicit NS)
BOOST_AUTO_UNIT_TEST( test_router_flexml_1 )
{
    try
    {
        std::string xmlconf = "<?xml version=\"1.0\"?>\n"
            "<yp2 xmlns=\"http://indexdata.dk/yp2/config/1\">\n"
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
            "</yp2>\n";

        yp2::FactoryStatic factory;
        factory.add_creator("tfilter", filter_creator);
        yp2::RouterFleXML rflexml(xmlconf, factory);
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
            "<y:yp2 xmlns:y=\"http://indexdata.dk/yp2/config/1\">\n"
            "  <start route=\"start\"/>\n"
            "  <filters>\n"
            "    <filter id=\"front_default\" type=\"frontend_net\">\n"
            "      <port>@:210</port>\n";
        
        yp2::FactoryFilter factory;
        yp2::RouterFleXML rflexml(xmlconf_invalid, factory);
    }
    catch ( yp2::XMLError &e) {
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
            "<y:yp2 xmlns:y=\"http://indexdata.dk/yp2/config/1\">\n"
            "  <y:start route=\"start\"/>\n"
            "  <y:filters>\n"
            "    <y:filter id=\"front_default\" type=\"frontend_net\">\n"
            "      <port>@:210</port>\n"
            "    </y:filter>\n"
            "    <y:filter id=\"log_cout\" type=\"log\">\n"
            "      <message>my msg</message>\n"
            "    </y:filter>\n"
            "  </y:filters>\n"
            "  <y:routes>\n"  
            "    <y:route id=\"start\">\n"
            "      <y:filter refid=\"front_default\"/>\n"
            "      <y:filter refid=\"log_cout\"/>\n"
            "    </y:route>\n"
            "  </y:routes>\n"
            "</y:yp2>\n";
       
        yp2::FactoryStatic factory;
        yp2::RouterFleXML rflexml(xmlconf, factory);
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
            "<yp2 xmlns=\"http://indexdata.dk/yp2/config/1\">\n"
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
            "</yp2>\n";

        yp2::FactoryStatic factory;
        factory.add_creator("tfilter", filter_creator);
        yp2::RouterFleXML rflexml(xmlconf, factory);
    }
    catch ( yp2::FactoryFilter::NotFound &e) {
        std::cout << "yp2::FactoryFilter::NotFound: " << e.what() << "\n";
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
