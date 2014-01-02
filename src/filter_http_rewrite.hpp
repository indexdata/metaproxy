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

#ifndef FILTER_HTTP_REWRITE_HPP
#define FILTER_HTTP_REWRITE_HPP

#include <metaproxy/filter.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

namespace mp = metaproxy_1;

namespace metaproxy_1 {
    namespace filter {
        class HttpRewrite : public Base {
            class Phase;
            class Content;
            class Replace;
            class Within;
            class Event;
            boost::scoped_ptr<Phase> req_phase;
            boost::scoped_ptr<Phase> res_phase;
            void configure_phase(const xmlNode *ptr, Phase &phase);
        public:
            class Rule;
            typedef boost::shared_ptr<Rule> RulePtr;
            HttpRewrite();
            ~HttpRewrite();
            void process(metaproxy_1::Package & package) const;
            void configure(const xmlNode * ptr,
                    bool test_only, const char *path);
        };
    }
}

extern "C" {
    extern struct metaproxy_1_filter_struct metaproxy_1_filter_http_rewrite;
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

