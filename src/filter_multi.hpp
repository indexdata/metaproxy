/* $Id: filter_multi.hpp,v 1.1 2006-01-15 20:03:14 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef FILTER_MULTI_HPP
#define FILTER_MULTI_HPP

#include <stdexcept>
#include <list>
#include <map>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "filter.hpp"

namespace yp2 {
    namespace filter {
        class Multi : public Base {
            class Rep;
            class Frontend;
            class Map;
            class Set;
            class Backend;
            class BackendSet;
            typedef boost::shared_ptr<Backend> BackendPtr;
            typedef boost::shared_ptr<Frontend> FrontendPtr;
            typedef boost::shared_ptr<Package> PackagePtr;
            typedef std::map<std::string,Set>::iterator Sets_it;
        public:
            ~Multi();
            Multi();
            void process(yp2::Package & package) const;
            void configure(const xmlNode * ptr);
            void add_map_host2hosts(std::string host,
                                    std::list<std::string> hosts,
                                    std::string route);
        private:
            boost::scoped_ptr<Rep> m_p;
        };
    }
}

extern "C" {
    extern struct yp2_filter_struct yp2_filter_multi;
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
