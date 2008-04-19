/* This file is part of Metaproxy.
   Copyright (C) 2005-2008 Index Data

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
        Origin(std::string listen_host = "", unsigned int listen_port = 0);
        
        /// get function - right val in assignment
        std::string listen_host() const;

        /// set function - left val in assignment
        std::string & listen_host();
 
        /// get function - right val in assignment
        unsigned int listen_port() const;
        
        /// set function - left val in assignment
        unsigned int & listen_port();
 
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
        unsigned int m_origin_id;
        std::string m_listen_host;
        unsigned int m_listen_port;
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
