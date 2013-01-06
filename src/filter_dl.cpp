/* This file is part of Metaproxy.
   Copyright (C) 2005-2013 Index Data

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

#include "config.hpp"

#include <metaproxy/filter.hpp>
#include <metaproxy/package.hpp>

namespace mp = metaproxy_1;

namespace metaproxy_1 {
    namespace filter {
        class Filter_dl: public mp::filter::Base {
        public:
            void process(mp::Package & package) const;
            void configure(const xmlNode * ptr, bool test_only,
                           const char *path);
        };
    }
}

void mp::filter::Filter_dl::process(mp::Package & package) const
{
}

void mp::filter::Filter_dl::configure(const xmlNode * ptr, bool test_only,
                                      const char *path)
{
    mp::xml::check_empty(ptr);
}

static mp::filter::Base* filter_creator()
{
    return new mp::filter::Filter_dl;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_dl = {
        0,
        "dl",
        filter_creator
    };
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

