/* $Id: filter_log.cpp,v 1.8 2005-10-30 17:13:36 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */


#include "config.hpp"

#include "filter.hpp"
#include "router.hpp"
#include "package.hpp"

#include "util.hpp"
#include "filter_log.hpp"

#include <yaz/zgdu.h>
#include <yaz/log.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>


yp2::filter::Log::Log() {}
yp2::filter::Log::Log(const std::string &msg) : m_msg(msg) {}

void yp2::filter::Log::process(Package &package) const {

    Z_GDU *gdu;

    // getting timestamp for receiving of package
    boost::posix_time::ptime receive_time
        = boost::posix_time::microsec_clock::local_time();

    // scope for locking Ostream 
    { 
        boost::mutex::scoped_lock scoped_lock(m_log_mutex);
        std::cout << receive_time << " " << m_msg;
        std::cout << " request id=" << package.session().id();
        std::cout << " close=" 
                  << (package.session().is_closed() ? "yes" : "no")
                  << "\n";
        gdu = package.request().get();
        if (gdu)
        {
            yp2::odr odr(ODR_PRINT);
            z_GDU(odr, &gdu, 0, 0);
        }
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
        std::cout << send_time << " " << m_msg;
        std::cout << " response id=" << package.session().id();
        std::cout << " close=" 
                  << (package.session().is_closed() ? "yes " : "no ")
                  << "duration=" << duration      
                  << "\n";
            //<< "duration=" << duration.total_seconds() 
            //    << "." << duration.fractional_seconds()
            //      << "\n";
        gdu = package.response().get();
        if (gdu)
        {
            yp2::odr odr(ODR_PRINT);
            z_GDU(odr, &gdu, 0, 0);
        }
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
