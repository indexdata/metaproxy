/* $Id: test_router_flexml.cpp,v 1.8 2006-01-04 11:19:04 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"
#include <iostream>
#include <stdexcept>

#include "filter.hpp"
#include "router_flexml.hpp"
#include "filter_factory.hpp"

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;

class TFilter: public yp2::filter::Base {
public:
    void process(yp2::Package & package) const {};
};
    

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
            "    <filter id=\"log_cout\" type=\"log\">\n"
            "      <logfile>mylog.log</logfile>\n"
            "    </filter>\n"
            "  </filters>\n"
            "  <routes>\n"  
            "    <route id=\"start\">\n"
            "      <filter refid=\"front_default\"/>\n"
            "      <filter refid=\"log_cout\"/>\n"
            "    </route>\n"
            "  </routes>\n"
            "</yp2>\n";
        yp2::RouterFleXML rflexml(xmlconf);
    }
    catch ( yp2::RouterFleXML::XMLError &e) {
        std::cout << "XMLError: " << e.what() << "\n";
        BOOST_CHECK (false);
    }
    catch ( yp2::FilterFactoryException &e) {
        std::cout << "FilterFactoryException: " << e.what() << "\n";
        BOOST_CHECK (false);
    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
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
        
        yp2::RouterFleXML rflexml(xmlconf_invalid);
    }
    catch ( yp2::RouterFleXML::XMLError &e) {
        got_xml_error = true;
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
       
        yp2::RouterFleXML rflexml(xmlconf);
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
