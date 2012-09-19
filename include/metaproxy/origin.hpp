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

#ifndef YP2_ORIGIN_HPP
#define YP2_ORIGIN_HPP

#include <iosfwd>
#include <string>

namespace metaproxy_1 {
    class Origin;
}

namespace std {
    std::ostream& operator<<(std::ostream& os, const metaproxy_1::Origin& o);
}

namespace metaproxy_1 {

    class Origin {
    public:
        Origin();

        /// set client IP info - left val in assignment
        void set_tcpip_address(std::string addr, unsigned long id);

        /// set max sockets (for outgoing connections to a given target)
        void set_max_sockets(int max_sockets);

        /// set max sockets (for outgoing connections to a given target)
        int get_max_sockets();

        /// get tcpip address
        std::string get_address();

        void set_custom_session(const std::string &s);
    private:
        friend std::ostream&
        std::operator<<(std::ostream& os, const metaproxy_1::Origin& o);

        std::string m_address;
        unsigned int m_origin_id;
        int m_max_sockets;
        std::string m_custom_session;
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

