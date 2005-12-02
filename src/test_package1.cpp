/* $Id: test_package1.cpp,v 1.4 2005-12-02 12:21:07 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"
#include <iostream>
#include <stdexcept>

#include "package.hpp"

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;

BOOST_AUTO_UNIT_TEST( test_package1_1 )
{
    try {
        yp2::Package package1;

        yp2::Origin origin;
        yp2::Session session;
        yp2::Package package2(package1.session(), origin);

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
