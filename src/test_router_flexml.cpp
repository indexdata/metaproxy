/* $Id: test_router_flexml.cpp,v 1.5 2005-12-02 12:21:07 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"
#include <iostream>
#include <stdexcept>

#include "filter.hpp"
#include "router_flexml.hpp"

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;

class TFilter: public yp2::filter::Base {
public:
    void process(yp2::Package & package) const {};
};
    

BOOST_AUTO_UNIT_TEST( test_router_flexml_1 )
{
    try{
        
        std::string xmlconf = "<?xml version=\"1.0\"?>\n"
            "<yp2 xmlns=\"http://indexdata.dk/yp2/config/1\">\n"
            "<start route=\"start\"/>\n"
            "<filters>\n"
            "<filter id=\"front_default\" type=\"frontend-net\">\n"
            "<port>210</port>\n"
            "</filter>\n"
            "<filter id=\"log_cout\" type=\"log\">\n"
            "<logfile>mylog.log</logfile>\n"
            "</filter>\n"
            "</filters>\n"
            "<routes>\n"  
            "<route id=\"start\">\n"
            "<filter refid=\"front_default\"/>\n"
            "<filter refid=\"log_cout\"/>\n"
            "</route>\n"
            "</routes>\n"
            "</yp2>\n";
       
        yp2::RouterFleXML rflexml(xmlconf);
        
        
        BOOST_CHECK (true);
        
        //BOOST_CHECK_EQUAL(filter.name(), std::string("filter1"));
        
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
