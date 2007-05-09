/* $Id: filter_frontend_net.hpp,v 1.16 2007-05-09 21:23:09 adam Exp $
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

#ifndef FILTER_FRONTEND_NET_HPP
#define FILTER_FRONTEND_NET_HPP

#include <boost/scoped_ptr.hpp>

#include <stdexcept>
#include <vector>

#include "filter.hpp"

namespace metaproxy_1 {
    namespace filter {
        class FrontendNet : public Base {
            class Rep;
            boost::scoped_ptr<Rep> m_p;
        public:
            FrontendNet();
            ~FrontendNet();
            void process(metaproxy_1::Package & package) const;
            void configure(const xmlNode * ptr);
        public:
            /// set ports
            std::vector<std::string> &ports();
            // set liten duraction (number of seconcds to listen)
            int &listen_duration();
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
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
