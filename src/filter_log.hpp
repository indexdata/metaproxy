/* $Id: filter_log.hpp,v 1.10 2005-12-11 17:23:05 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef FILTER_LOG_HPP
#define FILTER_LOG_HPP

#include <boost/scoped_ptr.hpp>

#include "filter.hpp"

namespace yp2 {
    namespace filter {
        class Log : public Base {
            class Rep;
            boost::scoped_ptr<Rep> m_p;
        public:
            Log();
            Log(const std::string &x);
            ~Log();
            void process(yp2::Package & package) const;
        };
    }
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
