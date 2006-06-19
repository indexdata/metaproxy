/* $Id: filter_log.hpp,v 1.16 2006-06-19 13:08:00 adam Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#ifndef FILTER_LOG_HPP
#define FILTER_LOG_HPP

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "filter.hpp"

namespace metaproxy_1 {
    namespace filter {
        class Log : public Base {
            class Rep;
            boost::scoped_ptr<Rep> m_p;
        public:
            Log();
            Log(const std::string &x);
            ~Log();
            void process(metaproxy_1::Package & package) const;
            void configure(const xmlNode * ptr);
            class LFile;
        };
    }
}

extern "C" {
    extern struct metaproxy_1_filter_struct metaproxy_1_filter_log;
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
