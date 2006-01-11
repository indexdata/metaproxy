/* $Id: filter_log.cpp,v 1.14 2006-01-11 08:53:52 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"

#include "xmlutil.hpp"
#include "package.hpp"

#include <string>
#include <boost/thread/mutex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "util.hpp"
#include "filter_log.hpp"

#include <yaz/zgdu.h>

namespace yf = yp2::filter;

namespace yp2 {
    namespace filter {
        class Log::Rep {
            friend class Log;
            static boost::mutex m_log_mutex;
            std::string m_msg;
        };
    }
}

boost::mutex yf::Log::Rep::m_log_mutex;

yf::Log::Log(const std::string &x) : m_p(new Rep)
{
    m_p->m_msg = x;
}

yf::Log::Log() : m_p(new Rep)
{
}

yf::Log::~Log() {}

void yf::Log::process(yp2::Package &package) const
{
    Z_GDU *gdu;

    // getting timestamp for receiving of package
    boost::posix_time::ptime receive_time
        = boost::posix_time::microsec_clock::local_time();

    // scope for locking Ostream 
    { 
        boost::mutex::scoped_lock scoped_lock(Rep::m_log_mutex);
        std::cout << receive_time << " " << m_p->m_msg;
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
        boost::mutex::scoped_lock scoped_lock(Rep::m_log_mutex);
        std::cout << send_time << " " << m_p->m_msg;
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

void yf::Log::configure(const xmlNode *ptr)
{
    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *) ptr->name, "message"))
            m_p->m_msg = yp2::xml::get_text(ptr);
        else
        {
            throw yp2::filter::FilterException("Bad element " 
                                               + std::string((const char *)
                                                             ptr->name));
        }
    }
}

static yp2::filter::Base* filter_creator()
{
    return new yp2::filter::Log;
}

extern "C" {
    struct yp2_filter_struct yp2_filter_log = {
        0,
        "log",
        filter_creator
    };
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
