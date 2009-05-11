/* This file is part of Metaproxy.
   Copyright (C) 2005-2009 Index Data

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

// Filter that does nothing. Use as RecordTransform for new filters 
#ifndef FILTER_RECORD_TRANSFORM_HPP
#define FILTER_RECORD_TRANSFORM_HPP

#include <boost/scoped_ptr.hpp>

#include "filter.hpp"

namespace metaproxy_1 {
    namespace filter {
        class RecordTransform : public Base {
            class Impl;
            boost::scoped_ptr<Impl> m_p;
        public:
            RecordTransform();
            ~RecordTransform();
            void process(metaproxy_1::Package & package) const;
            void configure(const xmlNode * ptr, bool test_only);
        };
    }
}

extern "C" {
    extern struct metaproxy_1_filter_struct metaproxy_1_filter_record_transform;
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

