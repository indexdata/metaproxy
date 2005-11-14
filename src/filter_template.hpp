/* $Id: filter_template.hpp,v 1.1 2005-11-14 23:35:22 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef FILTER_TEMPLATE_HPP
#define FILTER_TEMPLATE_HPP

#include <boost/scoped_ptr.hpp>

#include "filter.hpp"

namespace yp2 {
    namespace filter {
        class Template : public Base {
            class Rep;
        public:
            ~Template();
            Template();
            void process(yp2::Package & package) const;
        private:
            boost::scoped_ptr<Rep> m_p;
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
