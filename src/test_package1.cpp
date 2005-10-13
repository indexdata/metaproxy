
#include "config.hpp"
#include <iostream>
#include <stdexcept>

#include "package.hpp"

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;

BOOST_AUTO_TEST_CASE( test_package1_1 )
{
    try {
        yp2::Origin origin;
        yp2::Session session;
        yp2::Package package1(session, origin);

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
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
