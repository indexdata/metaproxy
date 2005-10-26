/* $Id: test_router_flexml.cpp,v 1.1 2005-10-26 14:12:00 marc Exp $
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
    

BOOST_AUTO_TEST_CASE( test_router_flexml_1 )
{
    try{
        TFilter filter;

        std::string xmlconf = "<?xml version=\"1.0\"?>"
            "<yp2 xmlns=\"http://indexdata.dk/yp2/config/1\">"
            "<start route=\"start\"/>"
            "<filters>"
            "<filter id=\"front_default\" type=\"frontend-net\">"
            "<port>210</port>"
            "</filter>"
            "<filter id=\"log_cout\" type=\"log\">"
            "<logfile>mylog.log</logfile>"
            "</filter>"
            "</filters>"
            "<routes>"  
            "<route id=\"start\">"
            "<filter refid=\"front_default\"/>"
            "<filter refid=\"log_cout\"/>"
            "</route>"
            "</routes>"
            "</yp2>";
        
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
