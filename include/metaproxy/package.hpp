/* This file is part of Metaproxy.
   Copyright (C) 2005-2012 Index Data

Metaproxy is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Metaproxy is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef YP2_PACKAGE_HPP
#define YP2_PACKAGE_HPP

#include <iosfwd>

#include <yazpp/gdu.h>

#include "router.hpp"
#include "filter.hpp"
#include "session.hpp"
#include "origin.hpp"
#include <boost/shared_ptr.hpp>

namespace metaproxy_1 {
    class Package;
}


namespace std
{
    std::ostream& operator<<(std::ostream& os, const metaproxy_1::Package& p);
}

namespace metaproxy_1 {

    class Package {
    public:
        Package();

        ~Package();

        Package(metaproxy_1::Session &session,
                const metaproxy_1::Origin &origin);

        /// copy constructor which copies route pos + logger
        Package & copy_filter(const Package &p);

        /// copy constructor which only copies the filter chain info
        void copy_route(const Package &p);

        /// send Package to it's next Filter defined in Router
        void move();

        /// send Package to other route
        void move(std::string route);

        /// access session - left val in assignment
        metaproxy_1::Session & session();

        /// get function - right val in assignment
        Origin origin() const;

        /// set function - left val in assignment
        Origin & origin();

        /// set function - can be chained
        Package & router(const Router &router);

        yazpp_1::GDU &request();

        yazpp_1::GDU &response();

        /// get function - right val in assignment
        Session session() const;

        void log(const char *module, int level, const char *fmt, ...);
        void log_write(const char *buf, size_t sz);
        void log_enable(void);
        void log_reset(std::string &res);

        class PackageLogger;
        typedef boost::shared_ptr<PackageLogger> PackageLoggerPtr;

    private:
        Session m_session;
        Origin m_origin;

        RoutePos *m_route_pos;

        PackageLoggerPtr p_logger;

        yazpp_1::GDU m_request_gdu;
        yazpp_1::GDU m_response_gdu;
    };
}

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

