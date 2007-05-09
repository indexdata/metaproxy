/* $Id: package.hpp,v 1.27 2007-05-09 21:23:09 adam Exp $
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

#ifndef YP2_PACKAGE_HPP
#define YP2_PACKAGE_HPP

#include <iosfwd>

#include <yazpp/gdu.h>

#include "router.hpp"
#include "filter.hpp"
#include "session.hpp"
#include "origin.hpp"

namespace metaproxy_1 {
    class Package;
}


namespace std 
{
    std::ostream& operator<<(std::ostream& os, metaproxy_1::Package& p);
}

namespace metaproxy_1 {

    class Package {
    public:
        Package();

        ~Package();
        
        Package(metaproxy_1::Session &session, 
                const metaproxy_1::Origin &origin);

        /// shallow copy constructor which only copies the filter chain info
        Package & copy_filter(const Package &p);

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
        Package & origin(const Origin & origin);

        /// set function - can be chained
        Package & router(const Router &router);

        yazpp_1::GDU &request();

        yazpp_1::GDU &response();
                
        /// get function - right val in assignment
        Session session() const;
        
    private:
        Session m_session;
        Origin m_origin;

        RoutePos *m_route_pos;

        //int m_data;
        
        yazpp_1::GDU m_request_gdu;
        yazpp_1::GDU m_response_gdu;
    };
}



#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
