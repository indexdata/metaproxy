/* $Id: filter_virt_db.hpp,v 1.17 2006-09-29 08:42:47 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
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
            void configure(const xmlNode * ptr);
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
