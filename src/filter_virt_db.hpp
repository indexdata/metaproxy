/* $Id: filter_virt_db.hpp,v 1.20 2008-02-20 15:07:52 adam Exp $
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

#ifndef FILTER_VIRT_DB_HPP
#define FILTER_VIRT_DB_HPP

#include <stdexcept>
#include <list>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "filter.hpp"

namespace metaproxy_1 {
    namespace filter {
        class VirtualDB : public Base {
            class Rep;
            struct Frontend;
            struct Map;
            struct Set;
            struct Backend;
            typedef boost::shared_ptr<Backend> BackendPtr;
            typedef boost::shared_ptr<Frontend> FrontendPtr;
        public:
            ~VirtualDB();
            VirtualDB();
            void process(metaproxy_1::Package & package) const;
            void configure(const xmlNode * ptr, bool test_only);
            void add_map_db2targets(std::string db,
                                    std::list<std::string> targets,
                                    std::string route);
            void add_map_db2target(std::string db,
                                   std::string target,
                                   std::string route);
        private:
            boost::scoped_ptr<Rep> m_p;
        };
    }
}

extern "C" {
    extern struct metaproxy_1_filter_struct metaproxy_1_filter_virt_db;
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
