/* $Id: test_boost_time.cpp,v 1.10 2007-04-20 08:53:34 adam Exp $
   Copyright (c) 2005-2007, Index Data.

   See the LICENSE file for details
 */
#include <iostream>

#include "config.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;



BOOST_AUTO_UNIT_TEST( testboosttime1 ) 
{

    // test session 
    try {

        boost::posix_time::ptime now
            = boost::posix_time::microsec_clock::local_time();
        //std::cout << now << std::endl;
        
        sleep(1);
        
        boost::posix_time::ptime then
            = boost::posix_time::microsec_clock::local_time();
        //std::cout << then << std::endl;
        
        boost::posix_time::time_period period(now, then);
        //std::cout << period << std::endl;
        
        boost::posix_time::time_duration duration = then - now;
        //std::cout << duration << std::endl;
        
        BOOST_CHECK (duration.total_seconds() >= 1);
        BOOST_CHECK (duration.fractional_seconds() > 0);
        
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
