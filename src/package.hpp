/* $Id: package.hpp,v 1.26 2007-01-25 14:05:54 adam Exp $
   Copyright (c) 2005-2007, Index Data.

   See the LICENSE file for details
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
