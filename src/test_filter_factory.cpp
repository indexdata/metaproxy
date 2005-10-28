/* $Id: test_filter_factory.cpp,v 1.1 2005-10-28 10:35:30 marc Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */


#include <iostream>
#include <stdexcept>

#include "config.hpp"
#include "filter.hpp"
#include "filter_factory.hpp"


#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;

class XFilter: public yp2::filter::Base {
public:
    void process(yp2::Package & package) const {};
};

class YFilter: public yp2::filter::Base {
public:
    void process(yp2::Package & package) const {};
};
    

BOOST_AUTO_TEST_CASE( test_router_flexml_1 )
{
    try{
        
        yp2::filter::FilterFactory  ffactory;
        

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
