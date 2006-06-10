/* $Id: test_filter1.cpp,v 1.16 2006-06-10 14:29:12 adam Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#include "config.hpp"
#include <iostream>
#include <stdexcept>

#include "filter.hpp"

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;
namespace mp = metaproxy_1;

class TFilter: public mp::filter::Base {
public:
    void process(mp::Package & package) const {};
};
    

BOOST_AUTO_UNIT_TEST( test_filter1 )
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
