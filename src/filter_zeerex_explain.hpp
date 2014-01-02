/* This file is part of Metaproxy.
   Copyright (C) Index Data

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

// Filter that constructs valid ZeeRex Explain SRU responses
#ifndef FILTER_ZEEREX_EXPLAIN_HPP
#define FILTER_ZEEREX_EXPLAIN_HPP

#include <boost/scoped_ptr.hpp>

#include <metaproxy/filter.hpp>

namespace metaproxy_1 {
    namespace filter {
        class ZeeRexExplain : public Base {
            class Impl;
            boost::scoped_ptr<Impl> m_p;
        public:
            ZeeRexExplain();
            ~ZeeRexExplain();
            void configure(const xmlNode *xmlnode, bool test_only,
                           const char *path);
            void process(metaproxy_1::Package & package) const;
        };
    }
}

extern "C" {
    extern struct metaproxy_1_filter_struct metaproxy_1_filter_zeerex_explain;
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

