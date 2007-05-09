/* $Id: filter_dl.cpp,v 1.9 2007-05-09 21:23:09 adam Exp $
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

#include "config.hpp"

#include "filter.hpp"
#include "package.hpp"

namespace mp = metaproxy_1;

namespace metaproxy_1 {
    namespace filter {
        class Filter_dl: public mp::filter::Base {
        public:
            void process(mp::Package & package) const;
        };
    }
}

void mp::filter::Filter_dl::process(mp::Package & package) const
{
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
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
