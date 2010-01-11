/* This file is part of Metaproxy.
   Copyright (C) 2005-2010 Index Data

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

#ifndef FILTER_MULTI_HPP
#define FILTER_MULTI_HPP

#include <stdexcept>
#include <list>
#include <map>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <metaproxy/filter.hpp>

namespace metaproxy_1 {
    namespace filter {
        class Multi : public Base {
            class Rep;
            struct Frontend;
            struct Map;
            struct FrontendSet;
            struct Backend;
            struct BackendSet;
            struct ScanTermInfo;
            typedef std::list<ScanTermInfo> ScanTermInfoList;
            typedef boost::shared_ptr<Backend> BackendPtr;
            typedef boost::shared_ptr<Frontend> FrontendPtr;
            typedef boost::shared_ptr<Package> PackagePtr;
            typedef std::map<std::string,FrontendSet>::iterator Sets_it;
        public:
            ~Multi();
            Multi();
            void process(metaproxy_1::Package & package) const;
            void configure(const xmlNode * ptr, bool test_only);
            void add_map_host2hosts(std::string host,
                                    std::list<std::string> hosts,
                                    std::string route);
        private:
            boost::scoped_ptr<Rep> m_p;
        };
    }
}

extern "C" {
    extern struct metaproxy_1_filter_struct metaproxy_1_filter_multi;
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

