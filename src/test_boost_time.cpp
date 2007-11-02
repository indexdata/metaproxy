/* $Id: test_boost_time.cpp,v 1.12 2007-11-02 17:47:41 adam Exp $
   Copyright (c) 2005-2007, Index Data.

This file is part of Metaproxy.

Metaproxy is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Metaproxy is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Metaproxy; see the file LICENSE.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
 */

#include <iostream>

#include "config.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;



BOOST_AUTO_TEST_CASE( testboosttime1 ) 
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
