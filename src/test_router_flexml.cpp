/* $Id: test_router_flexml.cpp,v 1.11 2006-01-04 14:30:51 adam Exp $
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

static bool tfilter_destroyed = false;
class TFilter: public yp2::filter::Base {
public:
    void process(yp2::Package & package) const {};
    ~TFilter() { tfilter_destroyed = true; };
};

static yp2::filter::Base* filter_creator()
{
    return new TFilter;
}

BOOST_AUTO_UNIT_TEST( test_router_flexml_1 )
{
    try
    {
        std::string xmlconf = "<?xml version=\"1.0\"?>\n"
            "<yp2 xmlns=\"http://indexdata.dk/yp2/config/1\">\n"
            "  <start route=\"start\"/>\n"
            "  <filters>\n"
            "    <filter id=\"front_default\" type=\"frontend_net\">\n"
            "      <port>210</port>\n"
            "    </filter>\n"
            "    <filter id=\"log_cout1\" type=\"log\">\n"
            "      <logfile>mylog1.log</logfile>\n"
            "    </filter>\n"
            "    <filter id=\"tfilter_id\" type=\"tfilter\">\n"
            "      <someelement/>\n"
            "    </filter>\n"
            "    <filter id=\"log_cout2\" type=\"log\">\n"
            "      <logfile>mylog2.log</logfile>\n"
            "    </filter>\n"
            "  </filters>\n"
            "  <routes>\n"  
            "    <route id=\"start\">\n"
            "      <filter refid=\"front_default\"/>\n"
            "      <filter refid=\"log_cout\"/>\n"
            "    </route>\n"
            "  </routes>\n"
            "</yp2>\n";

        yp2::FactoryStatic factory;
        factory.add_creator("tfilter", filter_creator);
        yp2::RouterFleXML rflexml(xmlconf, factory);
    }
    catch ( std::runtime_error &e) {
        std::cout << "std::runtime error: " << e.what() << "\n";
        BOOST_CHECK (false);
    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
    BOOST_CHECK(tfilter_destroyed == true);
}

BOOST_AUTO_UNIT_TEST( test_router_flexml_2 )
{
    bool got_xml_error = false;
    try
    {
        std::string xmlconf_invalid = "<?xml version=\"1.0\"?>\n"
            "<y:yp2 xmlns:y=\"http://indexdata.dk/yp2/config/1\">\n"
            "  <start route=\"start\"/>\n"
            "  <filters>\n"
            "    <filter id=\"front_default\" type=\"frontend_net\">\n"
            "      <port>210</port>\n";
        
        yp2::FactoryFilter factory;
        yp2::RouterFleXML rflexml(xmlconf_invalid, factory);
    }
    catch ( yp2::RouterFleXML::XMLError &e) {
        got_xml_error = true;
    }
    catch ( std::runtime_error &e) {
        std::cout << "std::runtime error: " << e.what() << "\n";
        BOOST_CHECK (false);
    }
    catch ( ... ) {
        ;
    }
    BOOST_CHECK(got_xml_error);
}

BOOST_AUTO_UNIT_TEST( test_router_flexml_3 )
{
    try
    {
        std::string xmlconf = "<?xml version=\"1.0\"?>\n"
            "<y:yp2 xmlns:y=\"http://indexdata.dk/yp2/config/1\">\n"
            "  <y:start route=\"start\"/>\n"
            "  <y:filters>\n"
            "    <y:filter id=\"front_default\" type=\"frontend_net\">\n"
            "      <port>210</port>\n"
            "    </y:filter>\n"
            "    <y:filter id=\"log_cout\" type=\"log\">\n"
            "      <logfile>mylog.log</logfile>\n"
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


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
