/* $Id: filter_log.hpp,v 1.4 2005-10-19 21:44:01 marc Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef FILTER_LOG_HPP
#define FILTER_LOG_HPP

#include <stdexcept>
#include <vector>

#include "filter.hpp"

namespace yp2 {
    namespace filter {
        class Log : public Base {
        public:
            Log();
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
