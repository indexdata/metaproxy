/* $Id: origin.hpp,v 1.1 2006-09-26 13:04:07 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#ifndef YP2_ORIGIN_HPP
#define YP2_ORIGIN_HPP

#include <iosfwd>
#include <string>

namespace metaproxy_1 {
    class Origin;
}

namespace std {
    std::ostream& operator<<(std::ostream& os, metaproxy_1::Origin& o);
}

namespace metaproxy_1 {
    
    class Origin {
    public:
        Origin(std::string server_host = "", unsigned int server_port = 0);
        
        /// get function - right val in assignment
        std::string server_host() const;
        
        /// get function - right val in assignment
        unsigned int server_port() const;
        
        /// set client IP info - left val in assignment
        void set_tcpip_address(std::string addr, unsigned long id);

    private:
        friend std::ostream& 
        std::operator<<(std::ostream& os,  metaproxy_1::Origin& o);
        
        enum origin_t {
            API,
            UNIX,
            TCPIP
        } m_type;
        std::string m_address; // UNIX+TCPIP
        unsigned long m_origin_id;
        std::string m_server_host;
        unsigned int m_server_port;
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
