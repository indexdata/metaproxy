/* $Id: filter_template.hpp,v 1.4 2006-01-04 11:55:31 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

// Filter that does nothing. Use as template for new filters 
#ifndef FILTER_TEMPLATE_HPP
#define FILTER_TEMPLATE_HPP

#include <boost/scoped_ptr.hpp>

#include "filter.hpp"

namespace yp2 {
    namespace filter {
        class Template : public Base {
            class Rep;
            boost::scoped_ptr<Rep> m_p;
        public:
            Template();
            ~Template();
            void process(yp2::Package & package) const;
        };
    }
}

extern "C" {
    extern struct yp2_filter_struct yp2_filter_template;
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
