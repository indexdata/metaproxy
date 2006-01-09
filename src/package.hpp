/* $Id: package.hpp,v 1.12 2006-01-09 13:43:59 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef YP2_PACKAGE_HPP
#define YP2_PACKAGE_HPP

#include <iostream>
#include <stdexcept>
#include <yaz++/gdu.h>

#include "router.hpp"
#include "filter.hpp"
#include "session.hpp"

namespace yp2 {
    
    class Origin {
        enum origin_t {
            API,
            UNIX,
            TCPIP
        } type;
        std::string address; // UNIX+TCPIP
        int port;            // TCPIP only
    public:
        Origin() : type(API) {};
    };

    class Package {
    public:
        Package();

        ~Package();
        
        Package(yp2::Session &session, yp2::Origin &origin);

        Package & copy_filter(const Package &p);

        /// send Package to it's next Filter defined in Router
        void move();
        
        /// access session - left val in assignment
        yp2::Session & session();
        
        /// get function - right val in assignment
        int data() const;

        /// set function - left val in assignment
        int & data();
        
        /// set function - can be chained
        Package & data(const int & data);
        
        /// get function - right val in assignment
        Origin origin() const;
        
        /// set function - left val in assignment
        Origin & origin();
        
        /// set function - can be chained
        Package & origin(const Origin & origin);

        Package & router(const Router &router);

        yazpp_1::GDU &request();

        yazpp_1::GDU &response();
                
        /// get function - right val in assignment
        Session session() const;
        
    private:
        Session m_session;
        Origin m_origin;

#if ROUTE_POS
        RoutePos *m_route_pos;
#else
        const filter::Base *m_filter;
        const Router *m_router;
#endif
        int m_data;
        
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
