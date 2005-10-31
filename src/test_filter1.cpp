/* $Id: test_filter1.cpp,v 1.13 2005-10-31 09:40:18 marc Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"
#include <iostream>
#include <stdexcept>

#include "filter.hpp"

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;

class TFilter: public yp2::filter::Base {
public:
    void process(yp2::Package & package) const {};
};
    

BOOST_AUTO_TEST_CASE( test_filter1 )
{
    try{
        TFilter filter;

        
        BOOST_CHECK (sizeof(filter) > 0);
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
