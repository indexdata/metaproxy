/* $Id: filter_bounce.hpp,v 1.2 2007-01-25 14:05:54 adam Exp $
   Copyright (c) 2005-2007, Index Data.

   See the LICENSE file for details
 */

// Filter that bounces all requests packages
// No packages are ever passed to later filters in the chain  
#ifndef FILTER_BOUNCE_HPP
#define FILTER_BOUNCE_HPP

#include <boost/scoped_ptr.hpp>

#include "filter.hpp"

namespace metaproxy_1 {
    namespace filter {
        class Bounce : public Base {
            class Rep;
            boost::scoped_ptr<Rep> m_p;
        public:
            Bounce();
            ~Bounce();
            void process(metaproxy_1::Package & package) const;
        };
    }
}

extern "C" {
    extern struct metaproxy_1_filter_struct metaproxy_1_filter_bounce;
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
