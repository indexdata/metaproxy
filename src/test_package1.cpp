/* $Id: test_package1.cpp,v 1.6 2006-06-10 14:29:13 adam Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#include "config.hpp"
#include <iostream>
#include <stdexcept>

#include "package.hpp"

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;
namespace mp = metaproxy_1;

BOOST_AUTO_UNIT_TEST( test_package1_1 )
{
    try {
        mp::Package package1;

        mp::Origin origin;
        mp::Session session;
        mp::Package package2(package1.session(), origin);

        BOOST_CHECK_EQUAL(package1.session().id(), package2.session().id());
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
