/* This file is part of Metaproxy.
   Copyright (C) 2005-2011 Index Data

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

#ifndef FILTER_ZOOM_HPP
#define FILTER_ZOOM_HPP

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <metaproxy/filter.hpp>

namespace metaproxy_1 {
    namespace filter {
        class Zoom : public Base {
            class Impl;
            class Frontend;
            class Backend;
            class Searchable;
            boost::scoped_ptr<Impl> m_p;
            typedef boost::shared_ptr<Frontend> FrontendPtr;
            typedef boost::shared_ptr<Backend> BackendPtr;
            typedef boost::shared_ptr<Searchable> SearchablePtr;
        public:
            Zoom();
            ~Zoom();
            void process(metaproxy_1::Package & package) const;
            void configure(const xmlNode * ptr, bool test_only,
                           const char *path);
        };
    }
}

extern "C" {
    extern struct metaproxy_1_filter_struct metaproxy_1_filter_zoom;
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

