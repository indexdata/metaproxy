/* $Id: filter_virt_db.hpp,v 1.10 2006-01-13 15:09:35 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef FILTER_VIRT_DB_HPP
#define FILTER_VIRT_DB_HPP

#include <stdexcept>
#include <list>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "filter.hpp"

namespace yp2 {
    namespace filter {
        class Virt_db : public Base {
            class Rep;
            class Frontend;
            class Map;
            class Set;
            class Backend;
            typedef boost::shared_ptr<Virt_db::Backend> BackendPtr;
        public:
            ~Virt_db();
            Virt_db();
            void process(yp2::Package & package) const;
            void configure(const xmlNode * ptr);
            void add_map_db2vhost(std::string db, std::string vhost,
                                  std::string route);
        private:
            boost::scoped_ptr<Rep> m_p;
        };
    }
}

extern "C" {
    extern struct yp2_filter_struct yp2_filter_virt_db;
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
