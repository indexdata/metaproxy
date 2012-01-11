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

#ifndef FILTER_FRONTEND_NET_HPP
#define FILTER_FRONTEND_NET_HPP

#include <boost/scoped_ptr.hpp>

#include <stdexcept>
#include <vector>

#include <metaproxy/filter.hpp>

namespace metaproxy_1 {
    namespace filter {
        class FrontendNet : public Base {
            class Rep;
            class Port;
            boost::scoped_ptr<Rep> m_p;
        public:
            FrontendNet();
            ~FrontendNet();
            void process(metaproxy_1::Package & package) const;
            void configure(const xmlNode * ptr, bool test_only,
                           const char *path);
        public:
            /// set ports
            void set_ports(std::vector<Port> &ports);
            void set_ports(std::vector<std::string> &ports);
            // set liten duraction (number of seconcds to listen)
            void set_listen_duration(int d);
        };
    }
}

extern "C" {
    extern struct metaproxy_1_filter_struct metaproxy_1_filter_frontend_net;
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

