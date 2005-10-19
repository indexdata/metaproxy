/* $Id: filter_log.cpp,v 1.5 2005-10-19 22:45:59 marc Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */


#include "config.hpp"

#include "filter.hpp"
#include "router.hpp"
#include "package.hpp"

#include "filter_log.hpp"

#include <yaz/zgdu.h>
#include <yaz/log.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>


yp2::filter::Log::Log() {}

void yp2::filter::Log::process(Package &package) const {

    Z_GDU *gdu;

    // getting timestamp for receiving of package
    boost::posix_time::ptime receive_time
        = boost::posix_time::microsec_clock::local_time();

    // scope for locking Ostream 
    { 
        boost::mutex::scoped_lock scoped_lock(m_log_mutex);
        std::cout << receive_time << " ";
        std::cout << "request id=" << package.session().id();
        std::cout << " close=" 
                  << (package.session().is_closed() ? "yes" : "no")
                  << "\n";
    }
    
    gdu = package.request().get();
    if (gdu)
    {
	ODR odr = odr_createmem(ODR_PRINT);
	z_GDU(odr, &gdu, 0, 0);
	odr_destroy(odr);
    }

    // unlocked during move
    package.move();

    // getting timestamp for sending of package
    boost::posix_time::ptime send_time
        = boost::posix_time::microsec_clock::local_time();

    boost::posix_time::time_duration duration = send_time - receive_time;

    // scope for locking Ostream 
    { 
        boost::mutex::scoped_lock scoped_lock(m_log_mutex);
        std::cout << send_time << " ";
        std::cout << "response id=" << package.session().id();
        std::cout << " close=" 
                  << (package.session().is_closed() ? "yes " : "no ")
                  << "duration=" << duration      
                  << "\n";
            //<< "duration=" << duration.total_seconds() 
            //    << "." << duration.fractional_seconds()
            //      << "\n";
    }
    
    gdu = package.response().get();
    if (gdu)
    {
	ODR odr = odr_createmem(ODR_PRINT);
	z_GDU(odr, &gdu, 0, 0);
	odr_destroy(odr);
    }
}

// defining and initializing static members
boost::mutex yp2::filter::Log::m_log_mutex;

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
