
#ifndef PACKAGE_HPP
#define PACKAGE_HPP

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
        
        Package(yp2::Session &session, yp2::Origin &origin) 
            : m_session(session), m_origin(origin),
              m_filter(0), m_router(0), m_data(0)  {}
        
        /// send Package to it's next Filter defined in Router
        Package & move() {
            m_filter = m_router->move(m_filter, this);
            if (m_filter)
                return m_filter->process(*this);
            else
                return *this;
        }
        
        /// access session - left val in assignment
        yp2::Session & session() {
            return m_session;
        }
        
        /// get function - right val in assignment
        unsigned int data() const {
            return m_data;
        }
        
        /// set function - left val in assignment
        unsigned int & data() {
            return m_data;
        }
        
        /// set function - can be chained
        Package & data(const unsigned int & data){
            m_data = data;
            return *this;
        }
        
        
        /// get function - right val in assignment
        Origin origin() const {
            return m_origin;
        }
        
        /// set function - left val in assignment
        Origin & origin() {
            return m_origin;
        }
        
        /// set function - can be chained
        Package & origin(const Origin & origin){
            m_origin = origin;
            return *this;
        }
        
        Package & router(const Router &router){
            m_filter = 0;
            m_router = &router;
            return *this;
        }
                
    private:
        Session m_session;
        Origin m_origin;
        
        const Filter *m_filter;
        const Router *m_router;
        unsigned int m_data;
        
        yazpp_1::GDU m_request_gdu;
        yazpp_1::GDU m_response_gdu;
    };
}

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
