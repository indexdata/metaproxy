/* $Id: test_session1.cpp,v 1.10 2006-03-16 10:40:59 adam Exp $
   Copyright (c) 2005-2006, Index Data.

%LICENSE%
 */
#include "config.hpp"
#include "session.hpp"

#include <iostream>

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;
namespace mp = metaproxy_1;

BOOST_AUTO_UNIT_TEST( testsession1 ) 
{

    // test session 
    try {
        mp::Session session1;
        mp::Session session2;
        mp::Session session3;
        mp::Session session4;
        mp::Session session5;

        BOOST_CHECK_EQUAL (session5.id(), (unsigned long) 5);

        mp::Session session = session3;

        BOOST_CHECK_EQUAL (session.id(), (unsigned long) 3);
    }
    catch (std::exception &e) {
        std::cout << e.what() << "\n";
        BOOST_CHECK (false);
    }
    catch (...) {
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
